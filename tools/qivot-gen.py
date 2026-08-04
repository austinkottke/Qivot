#!/usr/bin/env python3
"""
Qivot Code Generator: Generate Qivot model headers from database schemas.
Supports real-world production databases with warnings for unsupported patterns.

Usage:
    python3 qivot-gen.py --db sqlite:mydb.db --output src/models.h
    python3 qivot-gen.py --db postgresql://user:pass@localhost/dbname --output src/models.h
    python3 qivot-gen.py --db mysql://user:pass@localhost/dbname --output src/models.h

Supported databases:
    - SQLite: sqlite:path/to/db.db
    - PostgreSQL: postgresql://[user[:password]@][host[:port]]/dbname
    - MySQL: mysql://[user[:password]@][host[:port]]/dbname
    - SQL Server: mssql+pyodbc://user:password@dsn_name
"""

import sys
import argparse
import re
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple, Set
from abc import ABC, abstractmethod


@dataclass
class Column:
    """Represents a database column."""
    name: str
    cxx_type: str
    original_sql_type: str = ""
    is_primary_key: bool = False
    is_unique: bool = False
    is_not_null: bool = False
    has_default: bool = False
    default_value: str = ""
    is_foreign_key: bool = False
    fk_table: str = ""
    fk_column: str = ""
    needs_converter: bool = False
    converter_hint: str = ""
    is_soft_delete: bool = False
    is_polymorphic_type: bool = False
    is_polymorphic_id: bool = False
    check_expression: str = ""
    constraint_name: str = ""

    @property
    def cpp_name(self) -> str:
        """Column name in camelCase for C++."""
        parts = self.name.split('_')
        return parts[0] + ''.join(p.capitalize() for p in parts[1:])

    def get_constraints(self) -> List[str]:
        """Get QiField constraint flags."""
        constraints = []
        if self.is_not_null and not self.is_primary_key:
            constraints.append("QiNotNull")
        if self.is_unique and not self.is_primary_key:
            constraints.append("QiUnique")
        if self.has_default and not self.is_primary_key:
            if self.default_value.upper() in ["CURRENT_TIMESTAMP", "CURRENT_DATE", "CURRENT_TIME"]:
                constraints.append(f'QiDefault("{self.default_value}")')
            elif self.default_value.isdigit():
                constraints.append(f'QiDefault({self.default_value})')
            else:
                constraints.append(f'QiDefault("{self.default_value}")')
        return constraints


@dataclass
class Table:
    """Represents a database table."""
    name: str
    columns: List[Column]
    warnings: List[str] = field(default_factory=list)
    pk_columns: List[str] = field(default_factory=list)

    @property
    def class_name(self) -> str:
        """Table name converted to PascalCase class name."""
        return ''.join(word.capitalize() for word in self.name.split('_'))

    @property
    def has_foreign_keys(self) -> bool:
        return any(col.is_foreign_key for col in self.columns)

    @property
    def has_composite_key(self) -> bool:
        """Check if table has composite primary key."""
        return len(self.pk_columns) > 1

    def get_columns_excluding_pk(self) -> List[Column]:
        """Get all columns except primary key."""
        return [col for col in self.columns if not col.is_primary_key]

    def detect_polymorphic_relationships(self) -> None:
        """Detect polymorphic relationship patterns (type + id columns)."""
        col_names = {col.name for col in self.columns}

        for col in self.columns:
            if col.name.endswith('_type'):
                base_name = col.name[:-5]  # Remove '_type'
                if f"{base_name}_id" in col_names:
                    col.is_polymorphic_type = True
                    # Mark the _id column too
                    for c in self.columns:
                        if c.name == f"{base_name}_id":
                            c.is_polymorphic_id = True
                    self.warnings.append(
                        f"⚠️  Polymorphic relationship detected: '{col.name}' + '{base_name}_id' "
                        f"— manual QiForeignKey implementation required"
                    )

    def detect_soft_deletes(self) -> None:
        """Detect soft delete patterns."""
        soft_delete_patterns = ['deleted_at', 'is_deleted', 'deletion_time']

        for col in self.columns:
            if col.name in soft_delete_patterns or col.name.lower() in soft_delete_patterns:
                col.is_soft_delete = True
                self.warnings.append(
                    f"✓ Soft delete column detected: '{col.name}' — consider using soft_deletes scope"
                )


class SchemaParser(ABC):
    """Abstract base class for database schema parsers."""

    def __init__(self):
        self.warnings: List[str] = []

    @abstractmethod
    def connect(self, connection_string: str) -> None:
        """Connect to the database."""
        pass

    @abstractmethod
    def disconnect(self) -> None:
        """Disconnect from the database."""
        pass

    @abstractmethod
    def get_tables(self) -> List[Table]:
        """Get all tables from the database."""
        pass

    def should_skip_table(self, table_name: str) -> bool:
        """Check if table should be skipped (FTS, views, system tables)."""
        # FTS tables
        if any(suffix in table_name.lower() for suffix in ['_fts', '_fts5', '_fts_data', '_fts_content', '_fts_config']):
            self.warnings.append(f"⊘ FTS table '{table_name}' skipped — use QiFtsIndex for full-text search")
            return True

        # System tables
        if table_name.startswith(('sqlite_', 'pg_', 'information_schema', 'mysql_', 'sys_')):
            return True

        return False

    def map_sql_type_to_cpp(self, sql_type: str) -> Tuple[str, bool, str]:
        """
        Map SQL type to C++ type.
        Returns: (cpp_type, needs_converter, converter_hint)
        """
        sql_type_upper = sql_type.upper().split('(')[0].strip()

        type_map = {
            'INTEGER': ('int', False, ''),
            'INT': ('int', False, ''),
            'BIGINT': ('qlonglong', False, ''),
            'SMALLINT': ('short', False, ''),
            'TINYINT': ('char', False, ''),
            'REAL': ('qreal', False, ''),
            'DOUBLE': ('qreal', False, ''),
            'FLOAT': ('qreal', False, ''),
            'NUMERIC': ('qreal', False, ''),
            'DECIMAL': ('qreal', False, ''),
            'TEXT': ('QString', False, ''),
            'VARCHAR': ('QString', False, ''),
            'CHAR': ('QString', False, ''),
            'NVARCHAR': ('QString', False, ''),
            'NCHAR': ('QString', False, ''),
            'CLOB': ('QString', False, ''),
            'DATE': ('QDate', False, ''),
            'DATETIME': ('QDateTime', False, ''),
            'TIMESTAMP': ('QDateTime', False, ''),
            'TIME': ('QTime', False, ''),
            'BLOB': ('QByteArray', False, ''),
            'BOOLEAN': ('bool', False, ''),
            'BOOL': ('bool', False, ''),
            'BIT': ('bool', False, ''),
            'JSON': ('QVariant', True, 'custom JSON serializer using QJsonDocument'),
            'JSONB': ('QVariant', True, 'custom JSON serializer using QJsonDocument'),
            'UUID': ('QString', False, 'consider using QUuid for UUID type'),
            'ARRAY': ('QVariant', True, 'PostgreSQL array type — consider custom container type'),
            'ENUM': ('QString', True, 'SQL ENUM — convert to enum or string'),
        }

        if sql_type_upper in type_map:
            return type_map[sql_type_upper]

        # Default to QVariant for unknown types
        return ('QVariant', True, f'unknown type "{sql_type}" — custom converter needed')


class SqliteParser(SchemaParser):
    """Parser for SQLite databases."""

    def __init__(self):
        super().__init__()
        self.conn = None
        self.cursor = None

    def connect(self, connection_string: str) -> None:
        """Connect to SQLite database (format: sqlite:path/to/db.db)."""
        try:
            import sqlite3
        except ImportError:
            raise ImportError("sqlite3 module not found (should be builtin)")

        if connection_string.startswith("sqlite:"):
            db_path = connection_string[7:]
        else:
            db_path = connection_string

        self.conn = sqlite3.connect(db_path)
        self.cursor = self.conn.cursor()

    def disconnect(self) -> None:
        """Disconnect from SQLite database."""
        if self.conn:
            self.conn.close()

    def get_tables(self) -> List[Table]:
        """Get all tables from SQLite database."""
        tables = []

        self.cursor.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"
        )
        table_names = [row[0] for row in self.cursor.fetchall()]

        for table_name in table_names:
            if self.should_skip_table(table_name):
                continue

            columns = self._get_columns(table_name)
            table = Table(name=table_name, columns=columns)

            # Set pk_columns for composite key detection
            table.pk_columns = [col.name for col in columns if col.is_primary_key]

            # Warn about composite keys
            if table.has_composite_key:
                table.warnings.append(
                    f"⚠️  Composite primary key detected: {table.pk_columns} "
                    f"— use QI_DECLARE_MODEL_NOID"
                )

            table.detect_polymorphic_relationships()
            table.detect_soft_deletes()
            self.warnings.extend(table.warnings)
            tables.append(table)

        return tables

    def _get_columns(self, table_name: str) -> List[Column]:
        """Get columns for a specific table."""
        columns = []

        self.cursor.execute(f"PRAGMA table_info({table_name})")
        col_info = self.cursor.fetchall()

        self.cursor.execute(f"PRAGMA primary_key({table_name})")
        pk_cols = [row[1] for row in self.cursor.fetchall()]

        self.cursor.execute(f"PRAGMA foreign_key_list({table_name})")
        fk_info = {row[3]: (row[2], row[4]) for row in self.cursor.fetchall()}

        # Get UNIQUE constraints from sqlite_master schema
        unique_cols = self._get_unique_constraints(table_name)

        for col_id, col_name, col_type, not_null, default_val, pk in col_info:
            is_pk = pk > 0
            is_fk = col_name in fk_info
            is_unique = col_name in unique_cols

            cpp_type, needs_converter, converter_hint = self.map_sql_type_to_cpp(col_type)

            col = Column(
                name=col_name,
                original_sql_type=col_type,
                cxx_type=cpp_type,
                is_primary_key=is_pk,
                is_unique=is_unique,
                is_not_null=bool(not_null),
                has_default=default_val is not None,
                default_value=str(default_val) if default_val else "",
                needs_converter=needs_converter,
                converter_hint=converter_hint,
            )

            if is_fk:
                col.is_foreign_key = True
                col.fk_table, col.fk_column = fk_info[col_name]

            columns.append(col)

        return columns

    def _get_unique_constraints(self, table_name: str) -> Set[str]:
        """Extract UNIQUE constraints from CREATE TABLE statement."""
        unique_cols = set()

        try:
            # Get the CREATE TABLE statement from sqlite_master
            self.cursor.execute(
                f"SELECT sql FROM sqlite_master WHERE type='table' AND name=?",
                (table_name,)
            )
            result = self.cursor.fetchone()

            if result and result[0]:
                sql = result[0].upper()
                # Look for column definitions with UNIQUE keyword
                import re
                # Match: COLUMN_NAME TYPE ... UNIQUE
                pattern = r'(\w+)\s+(TEXT|INTEGER|VARCHAR|REAL|BLOB|DATE|DATETIME|TIMESTAMP|NUMERIC|DECIMAL|BOOLEAN|BOOL|BIT|NVARCHAR|NCHAR|CLOB)\s+([A-Z\s]*?)UNIQUE'
                for match in re.finditer(pattern, sql):
                    col_name = match.group(1).lower()
                    unique_cols.add(col_name)
        except Exception:
            pass

        return unique_cols


class PostgresParser(SchemaParser):
    """Parser for PostgreSQL databases."""

    def __init__(self):
        super().__init__()
        self.conn = None
        self.cursor = None

    def connect(self, connection_string: str) -> None:
        """Connect to PostgreSQL database."""
        try:
            import psycopg2
        except ImportError:
            raise ImportError("psycopg2 not installed. Install with: pip install psycopg2-binary")

        self.conn = psycopg2.connect(connection_string)
        self.cursor = self.conn.cursor()

    def disconnect(self) -> None:
        """Disconnect from PostgreSQL database."""
        if self.conn:
            self.conn.close()

    def get_tables(self) -> List[Table]:
        """Get all tables from PostgreSQL database."""
        tables = []

        self.cursor.execute("""
            SELECT table_name FROM information_schema.tables
            WHERE table_schema = 'public' AND table_type = 'BASE TABLE'
            ORDER BY table_name
        """)
        table_names = [row[0] for row in self.cursor.fetchall()]

        for table_name in table_names:
            if self.should_skip_table(table_name):
                continue

            columns = self._get_columns(table_name)
            table = Table(name=table_name, columns=columns)

            # Set pk_columns for composite key detection
            table.pk_columns = [col.name for col in columns if col.is_primary_key]

            # Warn about composite/natural keys
            if table.has_composite_key:
                table.warnings.append(
                    f"⚠️  Composite primary key detected: {table.pk_columns} "
                    f"— use QI_DECLARE_MODEL_NOID"
                )

            table.detect_polymorphic_relationships()
            table.detect_soft_deletes()
            self.warnings.extend(table.warnings)
            tables.append(table)

        return tables

    def _get_columns(self, table_name: str) -> List[Column]:
        """Get columns for a specific PostgreSQL table."""
        columns = []

        self.cursor.execute(f"""
            SELECT column_name, data_type, is_nullable, column_default
            FROM information_schema.columns
            WHERE table_schema = 'public' AND table_name = %s
            ORDER BY ordinal_position
        """, (table_name,))

        col_info = self.cursor.fetchall()

        pk_cols = self._get_primary_keys(table_name)
        fk_info = self._get_foreign_keys(table_name)
        unique_cols = self._get_unique_constraints(table_name)
        check_info = self._get_check_constraints(table_name)

        for col_name, data_type, is_nullable, default_val in col_info:
            is_pk = col_name in pk_cols
            is_fk = col_name in fk_info
            is_unique = col_name in unique_cols

            cpp_type, needs_converter, converter_hint = self.map_sql_type_to_cpp(data_type)

            col = Column(
                name=col_name,
                original_sql_type=data_type,
                cxx_type=cpp_type,
                is_primary_key=is_pk,
                is_unique=is_unique,
                is_not_null=is_nullable == 'NO',
                has_default=default_val is not None,
                default_value=str(default_val) if default_val else "",
                needs_converter=needs_converter,
                converter_hint=converter_hint,
            )

            if is_fk:
                col.is_foreign_key = True
                col.fk_table, col.fk_column = fk_info[col_name]

            if col_name in check_info:
                col.check_expression, col.constraint_name = check_info[col_name]

            columns.append(col)

        return columns

    def _get_primary_keys(self, table_name: str) -> Set[str]:
        """Get primary key columns for a table using pg_constraint."""
        try:
            self.cursor.execute("""
                SELECT a.attname FROM pg_constraint c
                JOIN pg_class t ON c.conrelid = t.oid
                JOIN pg_attribute a ON a.attrelid = t.oid AND a.attnum = ANY(c.conkey)
                WHERE t.relname = %s AND c.contype = 'p'
                ORDER BY a.attnum
            """, (table_name,))
            return {row[0] for row in self.cursor.fetchall()}
        except Exception:
            # Fallback to older pg_index method
            self.cursor.execute(f"""
                SELECT a.attname FROM pg_index i
                JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = ANY(i.indkey)
                WHERE i.indrelname = %s AND i.indisprimary
            """, (f"{table_name}_pkey",))
            return {row[0] for row in self.cursor.fetchall()}

    def _get_foreign_keys(self, table_name: str) -> Dict[str, Tuple[str, str]]:
        """Get foreign key info using pg_constraint for better coverage."""
        fk_dict = {}

        try:
            self.cursor.execute("""
                SELECT a.attname, t2.relname, a2.attname
                FROM pg_constraint c
                JOIN pg_class t1 ON c.conrelid = t1.oid
                JOIN pg_attribute a ON a.attrelid = t1.oid AND a.attnum = ANY(c.conkey)
                JOIN pg_class t2 ON c.confrelid = t2.oid
                JOIN pg_attribute a2 ON a2.attrelid = t2.oid AND a2.attnum = ANY(c.confkey)
                WHERE t1.relname = %s AND c.contype = 'f'
            """, (table_name,))

            for col_name, fk_table, fk_col in self.cursor.fetchall():
                fk_dict[col_name] = (fk_table, fk_col)
        except Exception:
            # Fallback to information_schema method
            self.cursor.execute(f"""
                SELECT kcu.column_name, ccu.table_name, ccu.column_name
                FROM information_schema.table_constraints AS tc
                JOIN information_schema.key_column_usage AS kcu ON tc.constraint_name = kcu.constraint_name
                JOIN information_schema.constraint_column_usage AS ccu ON ccu.constraint_name = tc.constraint_name
                WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_name = %s
            """, (table_name,))

            for col_name, fk_table, fk_col in self.cursor.fetchall():
                fk_dict[col_name] = (fk_table, fk_col)

        return fk_dict

    def _get_unique_constraints(self, table_name: str) -> Set[str]:
        """Get unique constraint columns."""
        unique_cols = set()

        try:
            self.cursor.execute("""
                SELECT a.attname
                FROM pg_constraint c
                JOIN pg_class t ON c.conrelid = t.oid
                JOIN pg_attribute a ON a.attrelid = t.oid AND a.attnum = ANY(c.conkey)
                WHERE t.relname = %s AND c.contype = 'u'
            """, (table_name,))

            unique_cols = {row[0] for row in self.cursor.fetchall()}
        except Exception:
            pass

        return unique_cols

    def _get_check_constraints(self, table_name: str) -> Dict[str, Tuple[str, str]]:
        """Get CHECK constraints with their expressions."""
        check_dict = {}

        try:
            self.cursor.execute("""
                SELECT a.attname, c.conname, pg_get_constraintdef(c.oid)
                FROM pg_constraint c
                JOIN pg_class t ON c.conrelid = t.oid
                JOIN pg_attribute a ON a.attrelid = t.oid AND a.attnum = ANY(c.conkey)
                WHERE t.relname = %s AND c.contype = 'c'
            """, (table_name,))

            for col_name, constraint_name, constraint_def in self.cursor.fetchall():
                # Extract the expression from "CHECK (expression)"
                expr = constraint_def.replace('CHECK ', '').strip('()')
                check_dict[col_name] = (expr, constraint_name)
        except Exception:
            pass

        return check_dict


class MysqlParser(SchemaParser):
    """Parser for MySQL/MariaDB databases."""

    def __init__(self):
        super().__init__()
        self.conn = None
        self.cursor = None

    def connect(self, connection_string: str) -> None:
        """Connect to MySQL database."""
        try:
            import mysql.connector
        except ImportError:
            raise ImportError("mysql-connector-python not installed. Install with: pip install mysql-connector-python")

        match = re.match(r'mysql://([^:]+):([^@]+)@([^:/]+)(?::(\d+))?/(.+)', connection_string)
        if match:
            user, password, host, port, database = match.groups()
            self.conn = mysql.connector.connect(
                user=user,
                password=password,
                host=host,
                port=int(port) if port else 3306,
                database=database
            )
            self.cursor = self.conn.cursor(dictionary=True)
        else:
            raise ValueError(f"Invalid MySQL connection string: {connection_string}")

    def disconnect(self) -> None:
        """Disconnect from MySQL database."""
        if self.conn:
            self.conn.close()

    def get_tables(self) -> List[Table]:
        """Get all tables from MySQL database."""
        tables = []

        self.cursor.execute("SELECT TABLE_NAME FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE()")
        table_names = [row['TABLE_NAME'] for row in self.cursor.fetchall()]

        for table_name in table_names:
            if self.should_skip_table(table_name):
                continue

            columns = self._get_columns(table_name)
            table = Table(name=table_name, columns=columns)

            # Set pk_columns for composite key detection
            table.pk_columns = [col.name for col in columns if col.is_primary_key]

            # Warn about composite keys
            if table.has_composite_key:
                table.warnings.append(
                    f"⚠️  Composite primary key detected: {table.pk_columns} "
                    f"— use QI_DECLARE_MODEL_NOID"
                )

            table.detect_polymorphic_relationships()
            table.detect_soft_deletes()
            self.warnings.extend(table.warnings)
            tables.append(table)

        return tables

    def _get_columns(self, table_name: str) -> List[Column]:
        """Get columns for a specific MySQL table."""
        columns = []

        self.cursor.execute(f"DESCRIBE {table_name}")
        col_info = self.cursor.fetchall()

        for col_data in col_info:
            col_name = col_data['Field']
            col_type = col_data['Type']
            is_pk = col_data['Key'] == 'PRI'
            is_unique = col_data['Key'] == 'UNI'
            is_not_null = col_data['Null'] == 'NO'
            default_val = col_data['Default']

            cpp_type, needs_converter, converter_hint = self.map_sql_type_to_cpp(col_type)

            col = Column(
                name=col_name,
                original_sql_type=col_type,
                cxx_type=cpp_type,
                is_primary_key=is_pk,
                is_unique=is_unique,
                is_not_null=is_not_null,
                has_default=default_val is not None,
                default_value=str(default_val) if default_val else "",
                needs_converter=needs_converter,
                converter_hint=converter_hint,
            )

            columns.append(col)

        # Get FK info separately using INFORMATION_SCHEMA
        self.cursor.execute(f"""
            SELECT COLUMN_NAME, REFERENCED_TABLE_SCHEMA, REFERENCED_TABLE_NAME, REFERENCED_COLUMN_NAME
            FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE
            WHERE TABLE_NAME = %s AND REFERENCED_TABLE_NAME IS NOT NULL
        """, (table_name,))

        for fk_info in self.cursor.fetchall():
            col_name = fk_info['COLUMN_NAME']
            fk_schema = fk_info['REFERENCED_TABLE_SCHEMA']
            fk_table = fk_info['REFERENCED_TABLE_NAME']
            fk_column = fk_info['REFERENCED_COLUMN_NAME']

            for col in columns:
                if col.name == col_name:
                    col.is_foreign_key = True
                    col.fk_table = fk_table
                    col.fk_column = fk_column
                    break

        return columns


class CodeGenerator:
    """Generates C++ Qivot model code from tables."""

    def __init__(self):
        self.tables: List[Table] = []
        self.all_warnings: List[str] = []

    def add_tables(self, tables: List[Table]) -> None:
        """Add tables to generate code for."""
        self.tables.extend(tables)

    def add_warnings(self, warnings: List[str]) -> None:
        """Add parser warnings."""
        self.all_warnings.extend(warnings)

    def generate_header(self) -> str:
        """Generate complete C++ header file."""
        lines = [
            "#pragma once",
            "",
            "#include <qivot.h>",
            "#include <QString>",
            "#include <QDateTime>",
            "#include <QDate>",
            "#include <QTime>",
            "#include <QVariant>",
            "",
            "// Generated by qivot-gen.py",
            "// Customize this file as needed",
            "//",
            "// NOTE: Review marked fields below for patterns requiring custom implementation:",
            "//  - JSON/JSONB fields: Create custom QI_DECLARE_CONVERTER for serialization",
            "//  - Polymorphic relationships: Implement dynamic foreign key resolution",
            "//  - Soft deletes: Use soft_deletes scope in queries",
            "",
        ]

        # Generate forward declarations for FK tables
        fk_tables = set()
        for table in self.tables:
            for col in table.columns:
                if col.is_foreign_key:
                    fk_tables.add(col.fk_table)

        for fk_table_name in sorted(fk_tables):
            class_name = ''.join(word.capitalize() for word in fk_table_name.split('_'))
            lines.append(f"class {class_name};")

        if fk_tables:
            lines.append("")

        # Generate model classes
        for table in self.tables:
            lines.extend(self._generate_model_class(table))
            lines.append("")

        return '\n'.join(lines)

    def _generate_model_class(self, table: Table) -> List[str]:
        """Generate a single model class."""
        lines = []

        # Add comment for composite keys
        if table.has_composite_key:
            lines.append(f"// ⚠️  Composite key: {', '.join(table.pk_columns)} — use QI_DECLARE_MODEL_NOID")
            lines.append("")

        # Class declaration
        lines.append(f"class {table.class_name} : public QiModel {{")
        lines.append("    QI_MODEL")
        lines.append("public:")

        # Generate fields
        for col in table.get_columns_excluding_pk():
            # Add comments for special fields
            comment_parts = []

            if col.is_soft_delete:
                comment_parts.append("soft delete")
            if col.is_unique:
                comment_parts.append("unique")
            if col.check_expression:
                comment_parts.append(f"CHECK: {col.check_expression}")
            if col.needs_converter:
                comment_parts.append(f"converter: {col.converter_hint}")
            if col.is_polymorphic_type:
                comment_parts.append("polymorphic type")
            if col.is_polymorphic_id:
                comment_parts.append("polymorphic id")

            comment = f"  // {', '.join(comment_parts)}" if comment_parts else ""

            if col.is_foreign_key:
                fk_class_name = ''.join(word.capitalize() for word in col.fk_table.split('_'))
                lines.append(f"    QiForeignKey<{fk_class_name}> {col.cpp_name};{comment}")
            else:
                lines.append(f"    QiField<{col.cxx_type}> {col.cpp_name};{comment}")

        lines.append("};")

        # Generate QI_DECLARE_MODEL macro call
        if table.has_composite_key:
            lines.append(f'// QI_DECLARE_MODEL_NOID({table.class_name}, "{table.name}",')
        else:
            lines.append(f'QI_DECLARE_MODEL({table.class_name}, "{table.name}",')

        field_lines = []
        for col in table.get_columns_excluding_pk():
            constraints = col.get_constraints()
            if constraints:
                constraint_str = ' | '.join(constraints)
                field_lines.append(f'                 QI_FIELD({col.cpp_name}, {constraint_str})')
            else:
                field_lines.append(f'                 QI_FIELD({col.cpp_name})')

        lines.append(',\n'.join(field_lines))

        if table.has_composite_key:
            lines.append(");  // Uncomment when ready and remove ID field handling")
        else:
            lines.append(");")

        return lines


def get_parser(connection_string: str) -> SchemaParser:
    """Get appropriate parser based on connection string."""
    if connection_string.startswith("sqlite:"):
        return SqliteParser()
    elif connection_string.startswith("postgresql://"):
        return PostgresParser()
    elif connection_string.startswith("mysql://"):
        return MysqlParser()
    else:
        raise ValueError(f"Unsupported database: {connection_string}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate Qivot C++ model headers from database schemas"
    )
    parser.add_argument(
        "--db",
        required=True,
        help="Database connection string (sqlite:db.db, postgresql://..., mysql://...)"
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Output file path for generated header"
    )

    args = parser.parse_args()

    try:
        # Parse schema
        db_parser = get_parser(args.db)
        print(f"Connecting to {args.db}...", file=sys.stderr)

        try:
            db_parser.connect(args.db)
        except ImportError as e:
            print(f"Error: Missing database driver. {e}", file=sys.stderr)
            return 1
        except Exception as e:
            print(f"Error: Connection failed. {e}", file=sys.stderr)
            return 1

        print("Reading schema...", file=sys.stderr)
        tables = db_parser.get_tables()
        db_parser.disconnect()

        if not tables:
            print("⚠️  No tables found in database", file=sys.stderr)
            return 1

        print(f"Found {len(tables)} table(s)", file=sys.stderr)

        # Print warnings
        if db_parser.warnings:
            print("\n⚠️  Schema analysis warnings:", file=sys.stderr)
            for warning in db_parser.warnings:
                print(f"  {warning}", file=sys.stderr)

        # Generate code
        gen = CodeGenerator()
        gen.add_tables(tables)
        gen.add_warnings(db_parser.warnings)
        header_code = gen.generate_header()

        # Write output
        try:
            with open(args.output, 'w') as f:
                f.write(header_code)
        except IOError as e:
            print(f"Error: Cannot write to {args.output}. {e}", file=sys.stderr)
            return 1

        print(f"\nGenerated {args.output}", file=sys.stderr)
        print(f"  - {len(tables)} model class(es)", file=sys.stderr)

        # Count special fields
        json_fields = sum(1 for t in tables for c in t.columns if c.needs_converter and 'JSON' in c.original_sql_type.upper())
        fk_relations = sum(1 for t in tables for c in t.columns if c.is_foreign_key)
        polymorphic = sum(1 for t in tables for c in t.columns if c.is_polymorphic_type or c.is_polymorphic_id)
        composite_keys = sum(1 for t in tables if t.has_composite_key)
        unique_constraints = sum(1 for t in tables for c in t.columns if c.is_unique)
        check_constraints = sum(1 for t in tables for c in t.columns if c.check_expression)

        if fk_relations:
            print(f"  - {fk_relations} foreign key relationship(s)", file=sys.stderr)
        if json_fields:
            print(f"  - {json_fields} JSON field(s) requiring custom converter", file=sys.stderr)
        if polymorphic:
            print(f"  - {polymorphic} polymorphic relationship field(s)", file=sys.stderr)
        if composite_keys:
            print(f"  - {composite_keys} table(s) with composite key(s)", file=sys.stderr)
        if unique_constraints:
            print(f"  - {unique_constraints} unique constraint(s)", file=sys.stderr)
        if check_constraints:
            print(f"  - {check_constraints} CHECK constraint(s)", file=sys.stderr)

        return 0

    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc(file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
