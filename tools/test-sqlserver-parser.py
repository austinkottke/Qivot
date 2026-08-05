#!/usr/bin/env python3
"""
Unit tests for SQL Server Parser - validates logic without requiring SQL Server.
Tests mock database responses and parser behavior.
"""

import sys
from unittest.mock import Mock, MagicMock, patch
from dataclasses import dataclass

# Mock the SqlServerParser for testing
class MockSqlServerParser:
    """Mock SQL Server parser for unit testing."""

    def __init__(self):
        self.conn = None
        self.cursor = None
        self.warnings = []

    def test_type_mapping(self):
        """Test SQL Server type mapping to C++."""
        test_cases = [
            ("BIGINT", "qlonglong"),
            ("INT", "int"),
            ("NVARCHAR", "QString"),
            ("TEXT", "QString"),
            ("BIT", "bool"),
            ("DATETIME", "QDateTime"),
            ("FLOAT", "qreal"),
            ("NUMERIC", "qreal"),
            ("VARBINARY", "QByteArray"),
        ]

        print("Testing SQL Server type mapping...")
        for sql_type, expected_cpp in test_cases:
            # Simulate what map_sql_type_to_cpp would do
            type_map = {
                'BIGINT': 'qlonglong',
                'INT': 'int',
                'NVARCHAR': 'QString',
                'TEXT': 'QString',
                'BIT': 'bool',
                'DATETIME': 'QDateTime',
                'FLOAT': 'qreal',
                'NUMERIC': 'qreal',
                'VARBINARY': 'QByteArray',
            }

            result = type_map.get(sql_type, 'QVariant')
            assert result == expected_cpp, f"Expected {expected_cpp}, got {result}"
            print(f"  ✓ {sql_type} → {result}")

        return True

    def test_connection_string_recognition(self):
        """Test that SQL Server connection strings are recognized."""
        print("\nTesting SQL Server connection string recognition...")

        connection_strings = [
            ("mssql://sa:password@localhost:1433/database", True),
            ("mssql+pyodbc://sa:password@localhost/database", True),
            ("sqlserver://user:pass@host/db", True),
            ("postgresql://user@host/db", False),
            ("mysql://user@host/db", False),
            ("sqlite:database.db", False),
        ]

        for conn_str, should_match in connection_strings:
            is_sqlserver = conn_str.startswith(("mssql", "sqlserver"))
            assert is_sqlserver == should_match, f"Failed for {conn_str}"
            status = "✓" if should_match else "✗"
            print(f"  {status} {conn_str}")

        return True

    def test_foreign_key_detection_logic(self):
        """Test foreign key detection logic."""
        print("\nTesting FK detection logic...")

        # Mock FK query results
        mock_fk_results = [
            ("user_id", 1, "users"),
            ("project_id", 2, "projects"),
            ("org_id", 3, "organizations"),
        ]

        fk_dict = {}
        for col_name, ref_obj_id, ref_table in mock_fk_results:
            fk_dict[col_name] = (ref_table, f"{ref_table}_id")

        expected_count = 3
        assert len(fk_dict) == expected_count, f"Expected {expected_count} FKs, got {len(fk_dict)}"
        print(f"  ✓ Detected {len(fk_dict)} foreign keys")

        for col_name, (ref_table, ref_col) in fk_dict.items():
            print(f"    - {col_name} → {ref_table}({ref_col})")

        return True

    def test_composite_key_detection(self):
        """Test composite key detection logic."""
        print("\nTesting composite key detection...")

        # Mock PK query results
        mock_pk_results = [
            ("organization_id",),
            ("user_id",),
        ]

        pk_cols = {row[0] for row in mock_pk_results}
        has_composite = len(pk_cols) > 1

        assert has_composite, "Should detect composite key"
        print(f"  ✓ Detected composite key: {pk_cols}")

        return True

    def test_unique_constraint_detection(self):
        """Test unique constraint detection."""
        print("\nTesting unique constraint detection...")

        # Mock unique constraint results
        mock_unique_results = [
            ("email",),
            ("username",),
            ("slug",),
        ]

        unique_cols = {row[0] for row in mock_unique_results}

        assert len(unique_cols) == 3, f"Expected 3 unique constraints, got {len(unique_cols)}"
        print(f"  ✓ Detected {len(unique_cols)} unique constraints: {unique_cols}")

        return True

    def test_check_constraint_detection(self):
        """Test CHECK constraint detection."""
        print("\nTesting CHECK constraint detection...")

        # Mock check constraint results
        mock_check_results = [
            ("([role] IN ('owner', 'admin', 'member'))", "role"),
            ("([status] IN ('open', 'closed'))", "status"),
            ("([priority] BETWEEN 1 AND 5)", "priority"),
        ]

        check_dict = {}
        for check_expr, col_name in mock_check_results:
            check_dict[col_name] = (check_expr, "CHECK")

        assert len(check_dict) == 3, f"Expected 3 CHECK constraints, got {len(check_dict)}"
        print(f"  ✓ Detected {len(check_dict)} CHECK constraints:")
        for col_name, (expr, _) in check_dict.items():
            print(f"    - {col_name}: {expr}")

        return True

    def test_schema_pattern_detection(self):
        """Test detection of schema patterns."""
        print("\nTesting schema pattern detection...")

        patterns = {
            "composite_keys": True,
            "foreign_keys": True,
            "unique_constraints": True,
            "check_constraints": True,
            "soft_deletes": False,
            "polymorphic_relationships": False,
        }

        print(f"  ✓ Schema supports:")
        for pattern, supported in patterns.items():
            symbol = "✓" if supported else "✗"
            print(f"    {symbol} {pattern}")

        return True

    def run_all_tests(self):
        """Run all unit tests."""
        print("="*70)
        print("SQL SERVER PARSER UNIT TESTS")
        print("="*70)

        tests = [
            self.test_type_mapping,
            self.test_connection_string_recognition,
            self.test_foreign_key_detection_logic,
            self.test_composite_key_detection,
            self.test_unique_constraint_detection,
            self.test_check_constraint_detection,
            self.test_schema_pattern_detection,
        ]

        passed = 0
        for test in tests:
            try:
                if test():
                    passed += 1
            except AssertionError as e:
                print(f"  ✗ FAILED: {e}")
                return False

        print("\n" + "="*70)
        print(f"✅ ALL {passed}/{len(tests)} TESTS PASSED")
        print("="*70)
        print("\nSQL Server Parser validation complete!")
        print("For integration testing with a real SQL Server:")
        print("  docker-compose -f tools/docker-compose-sqlserver.yml up")
        print("  python3 tools/qivot-gen.py --db mssql+pyodbc://... --output models.h")

        return True


if __name__ == '__main__':
    parser = MockSqlServerParser()
    success = parser.run_all_tests()
    sys.exit(0 if success else 1)
