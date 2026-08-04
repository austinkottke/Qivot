#!/usr/bin/env python3
"""
Qivot Code Generator: Generate Qivot model headers from database schemas.

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
from dataclasses import dataclass
from typing import List, Dict, Optional, Tuple
from abc import ABC, abstractmethod


@dataclass
class Column:
    """Represents a database column."""
    name: str
    cxx_type: str
    is_primary_key: bool = False
    is_unique: bool = False
    is_not_null: bool = False
    has_default: bool = False
    default_value: str = ""
    is_foreign_key: bool = False
    fk_table: str = ""
    fk_column: str = ""

    def pascal_case(s: str) -> str:
        """Convert snake_case to PascalCase."""
        return ''.join(word.capitalize() for word in s.split('_'))

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
            # Format default value as SQL
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

    @property
    def class_name(self) -> str:
        """Table name converted to PascalCase class name."""
        return ''.join(word.capitalize() for word in self.name.split('_'))

    @property
    def has_foreign_keys(self) -> bool:
        return any(col.is_foreign_key for col in self.columns)

    def get_columns_excluding_pk(self) -> List[Column]:
        """Get all columns except primary key."""
        return [col for col in self.columns if not col.is_primary_key]


class SchemaParser(ABC):
    """Abstract base class for database schema parsers."""

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

    def map_sql_type_to_cpp(self, sql_type: str) -> str:
        """Map SQL type to C++ type."""
        sql_type = sql_type.upper().split('(')[0].strip()

        type_map = {
            'INTEGER': 'int',
            'INT': 'int',
            'BIGINT': 'qlonglong',
            'SMALLINT': 'short',
            'TINYINT': 'char',
            'REAL': 'qreal',
            'DOUBLE': 'qreal',
            'FLOAT': 'qreal',
            'NUMERIC': 'qreal',
            'DECIMAL': 'qreal',
            'TEXT': 'QString',
            'VARCHAR': 'QString',
            'CHAR': 'QString',
            'NVARCHAR': 'QString',
            'NCHAR': 'QString',
            'CLOB': 'QString',
            'DATE': 'QDate',
            'DATETIME': 'QDateTime',
            'TIMESTAMP': 'QDateTime',
            'TIME': 'QTime',
            'BLOB': 'QByteArray',
            'BOOLEAN': 'bool',
            'BOOL': 'bool',
            'BIT': 'bool',
        }

        return type_map.get(sql_type, 'QVariant')


class SqliteParser(SchemaParser):
    """Parser for SQLite databases."""

    def __init__(self):
        self.conn = None
        self.cursor = None

    def connect(self, connection_string: str) -> None:
        """Connect to SQLite database (format: sqlite:path/to/db.db)."""
        try:
            import sqlite3
        except ImportError:
            raise ImportError("sqlite3 module not found (should be builtin)")

        # Parse connection string: sqlite:path/to/db.db
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

        # Get table names
        self.cursor.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"
        )
        table_names = [row[0] for row in self.cursor.fetchall()]

        for table_name in table_names:
            columns = self._get_columns(table_name)
            tables.append(Table(name=table_name, columns=columns))

        return tables

    def _get_columns(self, table_name: str) -> List[Column]:
        """Get columns for a specific table."""
        columns = []

        # Get column info via PRAGMA
        self.cursor.execute(f"PRAGMA table_info({table_name})")
        col_info = self.cursor.fetchall()

        # Get primary key info
        self.cursor.execute(f"PRAGMA primary_key({table_name})")
        pk_cols = [row[1] for row in self.cursor.fetchall()]

        # Get foreign key info
        self.cursor.execute(f"PRAGMA foreign_key_list({table_name})")
        fk_info = {row[3]: (row[2], row[4]) for row in self.cursor.fetchall()}

        for col_id, col_name, col_type, not_null, default_val, pk in col_info:
            is_pk = pk > 0
            is_fk = col_name in fk_info

            col = Column(
                name=col_name,
                cxx_type=self.map_sql_type_to_cpp(col_type),
                is_primary_key=is_pk,
                is_not_null=bool(not_null),
                has_default=default_val is not None,
                default_value=str(default_val) if default_val else "",
            )

            if is_fk:
                col.is_foreign_key = True
                col.fk_table, col.fk_column = fk_info[col_name]

            columns.append(col)

        return columns


class PostgresParser(SchemaParser):
    """Parser for PostgreSQL databases."""

    def __init__(self):
        self.conn = None
        self.cursor = None

    def connect(self, connection_string: str) -> None:
        """Connect to PostgreSQL database."""
        try:
            import psycopg2
        except ImportError:
            raise ImportError("psycopg2 not installed. Install with: pip install psycopg2-binary")

        # Parse connection string: postgresql://user:pass@host:port/dbname
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
            columns = self._get_columns(table_name)
            tables.append(Table(name=table_name, columns=columns))

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

        # Get PK and FK info
        pk_cols = self._get_primary_keys(table_name)
        fk_info = self._get_foreign_keys(table_name)

        for col_name, data_type, is_nullable, default_val in col_info:
            is_pk = col_name in pk_cols
            is_fk = col_name in fk_info

            col = Column(
                name=col_name,
                cxx_type=self.map_sql_type_to_cpp(data_type),
                is_primary_key=is_pk,
                is_not_null=is_nullable == 'NO',
                has_default=default_val is not None,
                default_value=str(default_val) if default_val else "",
            )

            if is_fk:
                col.is_foreign_key = True
                col.fk_table, col.fk_column = fk_info[col_name]

            columns.append(col)

        return columns

    def _get_primary_keys(self, table_name: str) -> List[str]:
        """Get primary key columns for a table."""
        self.cursor.execute(f"""
            SELECT a.attname FROM pg_index i
            JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = ANY(i.indkey)
            WHERE i.indrelname = %s AND i.indisprimary
        """, (f"{table_name}_pkey",))

        return [row[0] for row in self.cursor.fetchall()]

    def _get_foreign_keys(self, table_name: str) -> Dict[str, Tuple[str, str]]:
        """Get foreign key info for a table."""
        self.cursor.execute(f"""
            SELECT kcu.column_name, ccu.table_name, ccu.column_name
            FROM information_schema.table_constraints AS tc
            JOIN information_schema.key_column_usage AS kcu ON tc.constraint_name = kcu.constraint_name
            JOIN information_schema.constraint_column_usage AS ccu ON ccu.constraint_name = tc.constraint_name
            WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_name = %s
        """, (table_name,))

        return {row[0]: (row[1], row[2]) for row in self.cursor.fetchall()}


class MysqlParser(SchemaParser):
    """Parser for MySQL/MariaDB databases."""

    def __init__(self):
        self.conn = None
        self.cursor = None

    def connect(self, connection_string: str) -> None:
        """Connect to MySQL database."""
        try:
            import mysql.connector
        except ImportError:
            raise ImportError("mysql-connector-python not installed. Install with: pip install mysql-connector-python")

        # Parse connection string: mysql://user:pass@host:port/dbname
        # Simple parser - use mysql.connector.connect() for complex URLs
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
            columns = self._get_columns(table_name)
            tables.append(Table(name=table_name, columns=columns))

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

            col = Column(
                name=col_name,
                cxx_type=self.map_sql_type_to_cpp(col_type),
                is_primary_key=is_pk,
                is_unique=is_unique,
                is_not_null=is_not_null,
                has_default=default_val is not None,
                default_value=str(default_val) if default_val else "",
            )

            columns.append(col)

        # Get FK info separately
        self.cursor.execute(f"""
            SELECT COLUMN_NAME, REFERENCED_TABLE_NAME, REFERENCED_COLUMN_NAME
            FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE
            WHERE TABLE_NAME = %s AND REFERENCED_TABLE_NAME IS NOT NULL
        """, (table_name,))

        for fk_info in self.cursor.fetchall():
            col_name = fk_info['COLUMN_NAME']
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

    def add_tables(self, tables: List[Table]) -> None:
        """Add tables to generate code for."""
        self.tables.extend(tables)

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
            "",
            "// Generated by qivot-gen.py",
            "// Customize this file as needed",
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

        # Class declaration
        lines.append(f"class {table.class_name} : public QiModel {{")
        lines.append("    QI_MODEL")
        lines.append("public:")

        # Generate fields
        for col in table.get_columns_excluding_pk():
            if col.is_foreign_key:
                fk_class_name = ''.join(word.capitalize() for word in col.fk_table.split('_'))
                lines.append(f"    QiForeignKey<{fk_class_name}> {col.cpp_name};")
            else:
                lines.append(f"    QiField<{col.cxx_type}> {col.cpp_name};")

        lines.append("};")

        # Generate QI_DECLARE_MODEL macro call
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
        db_parser.connect(args.db)

        print("Reading schema...", file=sys.stderr)
        tables = db_parser.get_tables()
        db_parser.disconnect()

        if not tables:
            print("No tables found in database", file=sys.stderr)
            return 1

        print(f"Found {len(tables)} table(s)", file=sys.stderr)

        # Generate code
        gen = CodeGenerator()
        gen.add_tables(tables)
        header_code = gen.generate_header()

        # Write output
        with open(args.output, 'w') as f:
            f.write(header_code)

        print(f"Generated {args.output}", file=sys.stderr)
        print(f"  - {len(tables)} model class(es)", file=sys.stderr)

        return 0

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
