############################################################################
#
# MODULE:       Test of db.select
# AUTHOR(S):    Gulshan Kumar
# PURPOSE:      Test parsing and structure of CSV and JSON outputs
# COPYRIGHT:    (C) 2021-2025 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

"""Test parsing and structure of CSV and JSON outputs from db.select"""

import csv
import io
import itertools
import json
import os
import pathlib
import tempfile

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class DifficultValueTest(TestCase):
    """Test case for CSV and JSON parsing and structure with difficult values."""

    table_name = "test_db_select"

    @classmethod
    def setUpClass(cls):
        """Create table with difficult attribute values"""

        cls.runModule(
            "db.connect",
            driver="sqlite",
        )

        cls.runModule(
            "db.execute",
            sql=f"""
            CREATE TABLE {cls.table_name} (
                cat INTEGER,
                x DOUBLE PRECISION,
                y DOUBLE PRECISION,
                z DOUBLE PRECISION,
                owner_id INTEGER,
                place_name TEXT,
                num_buildings INTEGER
            );
            """,
        )

        cls.runModule(
            "db.execute",
            sql=f"""
            INSERT INTO {cls.table_name} VALUES
            (1, 17.46938776, 18.67346939, 143, 1, 'Big Hill', 2),
            (2, 20.93877551, 17.44897959, 125, 2, 'Small Hill', 2),
            (3, 18.89795918, 14.18367347, 130, 3, NULL, 3),
            (4, 18.89795918, 14.18367347, 130, 4, 'The "Great" Place', 3),
            (5, 15.91836735, 10.67346939, 126, 5, 'Joan''s Place', 3),
            (6, 15.91836735, 10.67346939, 126, 6, 'Bright Hall|BLDG209', 3),
            (7, 15.91836735, 10.67346939, 126, 7, 'Raleigh, NC, USA', 3),
            (8, 15.91836735, 10.67346939, 126, 8, 'Building: GeoLab[5]', 3),
            (9, 15.91836735, 10.67346939, 126, 9,
                '892 Long Street' || char(10) || 'Raleigh NC 29401', 3);
            """,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove test table"""
        cls.runModule("db.execute", sql=f"DROP TABLE {cls.table_name};")

    def test_json_loads(self):
        """Load JSON with difficult values"""

        text = gs.read_command(
            "db.select",
            table=self.table_name,
            format="json",
        )
        data = json.loads(text)

        column_info = data["info"]["columns"]

        self.assertEqual(column_info[0]["name"], "cat")
        self.assertEqual(column_info[0]["sql_type"], "INTEGER")
        self.assertTrue(column_info[0]["is_number"])

        self.assertEqual(column_info[1]["name"], "x")
        self.assertEqual(column_info[1]["sql_type"], "DOUBLE PRECISION")
        self.assertTrue(column_info[1]["is_number"])

        self.assertEqual(column_info[4]["name"], "owner_id")
        self.assertEqual(column_info[4]["sql_type"], "INTEGER")
        self.assertTrue(column_info[4]["is_number"])

        self.assertEqual(column_info[5]["name"], "place_name")
        self.assertEqual(column_info[5]["sql_type"], "TEXT")
        self.assertFalse(column_info[5]["is_number"])

        records = data["records"]

        self.assertIsNone(records[2]["place_name"])
        self.assertEqual(records[3]["place_name"], 'The "Great" Place')
        self.assertEqual(records[4]["place_name"], "Joan's Place")
        self.assertEqual(records[7]["place_name"], "Building: GeoLab[5]")
        self.assertEqual(records[8]["place_name"], "892 Long Street\nRaleigh NC 29401")

    def test_csv_loads(self):
        """Load CSV with difficult values with many separators"""

        for delimiter, null_value in itertools.product(
            [None, ",", ";", "\t", "|"], [None, "NULL"]
        ):
            text = gs.read_command(
                "db.select",
                table=self.table_name,
                format="csv",
                separator=delimiter,
                null_value=null_value,
            )

            if delimiter is None:
                delimiter = ","
            if null_value is None:
                null_value = ""

            io_string = io.StringIO(text)
            reader = csv.DictReader(
                io_string,
                delimiter=delimiter,
                quotechar='"',
                doublequote=True,
                lineterminator="\n",
                strict=True,
            )

            data = list(reader)

            self.assertEqual(data[2]["place_name"], null_value)
            self.assertEqual(data[3]["place_name"], 'The "Great" Place')
            self.assertEqual(data[4]["place_name"], "Joan's Place")
            self.assertEqual(data[5]["place_name"], "Bright Hall|BLDG209")
            self.assertEqual(data[6]["place_name"], "Raleigh, NC, USA")
            self.assertEqual(data[8]["place_name"], "892 Long Street\nRaleigh NC 29401")


class MultipleSQLInputTest(TestCase):
    """Test for Multiple SQL statements: JSON reading successfully"""

    table_name = "test_db_select"

    @classmethod
    def setUpClass(cls):
        cls.db_path = os.path.join(tempfile.gettempdir(), "test_db_select.sqlite")

        cls.runModule(
            "db.connect",
            driver="sqlite",
            database=cls.db_path,
        )

        cls.runModule(
            "db.execute",
            sql=f"""
            CREATE TABLE {cls.table_name} (
                cat INTEGER,
                place_name TEXT
            );
            """,
        )

        cls.runModule(
            "db.execute",
            sql=f"""
            INSERT INTO {cls.table_name} VALUES
            (1, 'Big Hill'),
            (2, 'Small Hill');
            """,
        )

    @classmethod
    def tearDownClass(cls):
        cls.runModule("db.execute", sql=f"DROP TABLE {cls.table_name};")

        if pathlib.Path(cls.db_path).exists():
            os.remove(cls.db_path)

    def test_multiple_sql_json(self):
        sql_queries = (
            f"SELECT place_name FROM {self.table_name} WHERE cat = 1;\n"
            f"SELECT place_name FROM {self.table_name} WHERE cat = 2;\n"
        )

        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_sql:
            tmp_sql.write(sql_queries)
            tmp_sql_path = tmp_sql.name

        text = gs.read_command(
            "db.select",
            input=tmp_sql_path,
            format="json",
        )

        data = json.loads(text)

        self.assertIsInstance(data, list)
        self.assertEqual(len(data), 2)
        self.assertEqual(data[0]["records"][0]["place_name"], "Big Hill")
        self.assertEqual(data[1]["records"][0]["place_name"], "Small Hill")

        os.remove(tmp_sql_path)


if __name__ == "__main__":
    test()
