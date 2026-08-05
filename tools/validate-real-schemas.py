#!/usr/bin/env python3
"""
Real-world Schema Validation for qivot-gen
Tests against production-like schemas: Mastodon, Discourse, GitLab
Validates FK detection, constraint extraction, and code generation quality.
"""

import subprocess
import sys
import time
import json
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List, Dict, Tuple
import psycopg2
from psycopg2 import sql

@dataclass
class SchemaStats:
    """Statistics about schema analysis."""
    name: str
    tables: int
    columns: int
    foreign_keys: int
    unique_constraints: int
    check_constraints: int
    json_fields: int
    indexes: int
    models_generated: int
    fks_detected: int
    constraints_detected: int

class RealSchemaValidator:
    """Validates qivot-gen against production-like schemas."""

    def __init__(self):
        self.schemas = {
            'mastodon': {
                'port': 5432,
                'db': 'mastodon',
                'user': 'postgres',
                'password': 'testpass'
            },
            'discourse': {
                'port': 5433,
                'db': 'discourse',
                'user': 'postgres',
                'password': 'testpass'
            },
            'gitlab': {
                'port': 5434,
                'db': 'gitlab',
                'user': 'postgres',
                'password': 'testpass'
            }
        }
        self.results: Dict[str, SchemaStats] = {}

    def start_docker(self) -> bool:
        """Start Docker containers."""
        print("🐳 Starting Docker containers...")
        script_dir = Path(__file__).parent
        compose_file = script_dir / 'docker-compose-validation.yml'

        if not compose_file.exists():
            print(f"ERROR: docker-compose file not found at {compose_file}")
            return False

        try:
            subprocess.run(
                ['docker-compose', '-f', str(compose_file), 'up', '-d'],
                check=True,
                capture_output=True
            )
            print("✓ Docker containers started")
            return True
        except subprocess.CalledProcessError as e:
            print(f"ERROR: Failed to start Docker: {e.stderr.decode()}")
            return False

    def wait_for_databases(self, timeout: int = 60) -> bool:
        """Wait for databases to be ready."""
        print("\n⏳ Waiting for databases to be ready...")
        start_time = time.time()

        for name, config in self.schemas.items():
            while time.time() - start_time < timeout:
                try:
                    conn = psycopg2.connect(
                        host='localhost',
                        port=config['port'],
                        database=config['db'],
                        user=config['user'],
                        password=config['password']
                    )
                    conn.close()
                    print(f"  ✓ {name} ready")
                    break
                except psycopg2.OperationalError:
                    time.sleep(1)
            else:
                print(f"ERROR: {name} database did not become ready within {timeout}s")
                return False

        return True

    def analyze_schema(self, schema_name: str, config: Dict) -> SchemaStats:
        """Analyze a schema and collect statistics."""
        print(f"\n📊 Analyzing {schema_name} schema...")

        try:
            conn = psycopg2.connect(
                host='localhost',
                port=config['port'],
                database=config['db'],
                user=config['user'],
                password=config['password']
            )
            cursor = conn.cursor()

            # Count tables
            cursor.execute("""
                SELECT COUNT(*) FROM information_schema.tables
                WHERE table_schema = 'public'
            """)
            tables = cursor.fetchone()[0]

            # Count columns
            cursor.execute("""
                SELECT COUNT(*) FROM information_schema.columns
                WHERE table_schema = 'public'
            """)
            columns = cursor.fetchone()[0]

            # Count FKs
            cursor.execute("""
                SELECT COUNT(*) FROM information_schema.table_constraints
                WHERE table_schema = 'public' AND constraint_type = 'FOREIGN KEY'
            """)
            foreign_keys = cursor.fetchone()[0]

            # Count unique constraints
            cursor.execute("""
                SELECT COUNT(*) FROM information_schema.table_constraints
                WHERE table_schema = 'public' AND constraint_type = 'UNIQUE'
            """)
            unique_constraints = cursor.fetchone()[0]

            # Count check constraints
            cursor.execute("""
                SELECT COUNT(*) FROM information_schema.table_constraints
                WHERE table_schema = 'public' AND constraint_type = 'CHECK'
            """)
            check_constraints = cursor.fetchone()[0]

            # Count JSONB/JSON columns
            cursor.execute("""
                SELECT COUNT(*) FROM information_schema.columns
                WHERE table_schema = 'public' AND data_type IN ('jsonb', 'json')
            """)
            json_fields = cursor.fetchone()[0]

            # Count indexes
            cursor.execute("""
                SELECT COUNT(*) FROM pg_indexes
                WHERE schemaname = 'public'
            """)
            indexes = cursor.fetchone()[0]

            conn.close()

            stats = SchemaStats(
                name=schema_name,
                tables=tables,
                columns=columns,
                foreign_keys=foreign_keys,
                unique_constraints=unique_constraints,
                check_constraints=check_constraints,
                json_fields=json_fields,
                indexes=indexes,
                models_generated=0,
                fks_detected=0,
                constraints_detected=0
            )

            print(f"  Tables: {tables}")
            print(f"  Columns: {columns}")
            print(f"  Foreign Keys: {foreign_keys}")
            print(f"  JSONB/JSON Fields: {json_fields}")
            print(f"  Unique Constraints: {unique_constraints}")
            print(f"  Check Constraints: {check_constraints}")

            return stats

        except Exception as e:
            print(f"ERROR analyzing {schema_name}: {e}")
            return None

    def generate_models(self, schema_name: str, config: Dict) -> Tuple[bool, int, int, int]:
        """Generate models using qivot-gen."""
        print(f"\n🔧 Generating models for {schema_name}...")

        connection_string = f"postgresql://postgres:testpass@localhost:{config['port']}/{config['db']}"
        output_file = Path(f"/tmp/{schema_name}_models.h")

        try:
            result = subprocess.run(
                ['python3', 'tools/qivot-gen.py',
                 '--db', connection_string,
                 '--output', str(output_file)],
                capture_output=True,
                text=True,
                timeout=30
            )

            if result.returncode != 0:
                print(f"ERROR generating models: {result.stderr}")
                return False, 0, 0, 0

            # Parse output to get stats
            output = result.stderr
            models_generated = output.count('class ')
            fks_detected = output.count('QiForeignKey')
            constraints_detected = output.count('unique') + output.count('CHECK')

            print(f"  ✓ Generated {models_generated} models")
            print(f"  ✓ Detected {fks_detected} foreign keys")
            print(f"  ✓ Detected {constraints_detected} constraints")

            return True, models_generated, fks_detected, constraints_detected

        except subprocess.TimeoutExpired:
            print("ERROR: Code generation timed out")
            return False, 0, 0, 0
        except Exception as e:
            print(f"ERROR generating models: {e}")
            return False, 0, 0, 0

    def generate_report(self) -> None:
        """Generate validation report."""
        print("\n" + "="*70)
        print("📋 REAL-WORLD SCHEMA VALIDATION REPORT")
        print("="*70)

        total_tables = 0
        total_fks = 0
        total_fks_detected = 0

        for stats in self.results.values():
            print(f"\n{stats.name.upper()}")
            print("-" * 70)
            print(f"  Tables: {stats.tables} | Columns: {stats.columns}")
            print(f"  Foreign Keys: {stats.foreign_keys} → Detected: {stats.fks_detected} ({self._percent(stats.fks_detected, stats.foreign_keys)}%)")
            print(f"  Unique Constraints: {stats.unique_constraints}")
            print(f"  Check Constraints: {stats.check_constraints}")
            print(f"  JSONB/JSON Fields: {stats.json_fields}")
            print(f"  Models Generated: {stats.models_generated}")

            total_tables += stats.tables
            total_fks += stats.foreign_keys
            total_fks_detected += stats.fks_detected

        print("\n" + "="*70)
        print(f"TOTALS: {total_tables} tables, {total_fks} FKs detected: {total_fks_detected} ({self._percent(total_fks_detected, total_fks)}%)")
        print("="*70)

        # Save detailed report
        report_path = Path('/tmp/schema_validation_report.json')
        with open(report_path, 'w') as f:
            json.dump([asdict(s) for s in self.results.values()], f, indent=2)
        print(f"\n✓ Detailed report saved to {report_path}")

    def stop_docker(self) -> None:
        """Stop and remove Docker containers."""
        print("\n🛑 Cleaning up Docker containers...")
        script_dir = Path(__file__).parent
        compose_file = script_dir / 'docker-compose-validation.yml'

        try:
            subprocess.run(
                ['docker-compose', '-f', str(compose_file), 'down'],
                capture_output=True
            )
            print("✓ Docker containers stopped")
        except Exception as e:
            print(f"WARNING: Failed to stop Docker: {e}")

    @staticmethod
    def _percent(numerator: int, denominator: int) -> int:
        """Calculate percentage."""
        return int((numerator / denominator * 100) if denominator > 0 else 0)

    def run(self) -> int:
        """Run the full validation suite."""
        try:
            # Start Docker
            if not self.start_docker():
                return 1

            # Wait for databases
            if not self.wait_for_databases():
                self.stop_docker()
                return 1

            # Analyze and generate for each schema
            for schema_name, config in self.schemas.items():
                stats = self.analyze_schema(schema_name, config)
                if stats:
                    success, models, fks, constraints = self.generate_models(schema_name, config)
                    if success:
                        stats.models_generated = models
                        stats.fks_detected = fks
                        stats.constraints_detected = constraints
                        self.results[schema_name] = stats

            # Generate report
            self.generate_report()

            return 0

        finally:
            self.stop_docker()


if __name__ == '__main__':
    validator = RealSchemaValidator()
    sys.exit(validator.run())
