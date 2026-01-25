from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import parse_command, read_command

output_plain = """cat
OBJECTID
WAKE_ZIPCO
PERIMETER
ZIPCODE_
ZIPCODE_ID
ZIPNAME
ZIPNUM
ZIPCODE
NAME
SHAPE_Leng
SHAPE_Area
"""

output_plain_more_info = """cat: INTEGER
OBJECTID: INTEGER
WAKE_ZIPCO: DOUBLE PRECISION
PERIMETER: DOUBLE PRECISION
ZIPCODE_: DOUBLE PRECISION
ZIPCODE_ID: DOUBLE PRECISION
ZIPNAME: CHARACTER
ZIPNUM: DOUBLE PRECISION
ZIPCODE: CHARACTER
NAME: CHARACTER
SHAPE_Leng: DOUBLE PRECISION
SHAPE_Area: DOUBLE PRECISION
"""

output_json = [
    "cat",
    "OBJECTID",
    "WAKE_ZIPCO",
    "PERIMETER",
    "ZIPCODE_",
    "ZIPCODE_ID",
    "ZIPNAME",
    "ZIPNUM",
    "ZIPCODE",
    "NAME",
    "SHAPE_Leng",
    "SHAPE_Area",
]

output_json_more_info = [
    {"name": "cat", "sql_type": "INTEGER", "is_number": True},
    {"name": "OBJECTID", "sql_type": "INTEGER", "is_number": True},
    {"name": "WAKE_ZIPCO", "sql_type": "DOUBLE PRECISION", "is_number": True},
    {"name": "PERIMETER", "sql_type": "DOUBLE PRECISION", "is_number": True},
    {"name": "ZIPCODE_", "sql_type": "DOUBLE PRECISION", "is_number": True},
    {"name": "ZIPCODE_ID", "sql_type": "DOUBLE PRECISION", "is_number": True},
    {"name": "ZIPNAME", "sql_type": "CHARACTER", "is_number": False},
    {"name": "ZIPNUM", "sql_type": "DOUBLE PRECISION", "is_number": True},
    {"name": "ZIPCODE", "sql_type": "CHARACTER", "is_number": False},
    {"name": "NAME", "sql_type": "CHARACTER", "is_number": False},
    {"name": "SHAPE_Leng", "sql_type": "DOUBLE PRECISION", "is_number": True},
    {"name": "SHAPE_Area", "sql_type": "DOUBLE PRECISION", "is_number": True},
]


class TestDbColumns(TestCase):
    invect = "zipcodes"
    mapset = "$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db"

    @classmethod
    def setUpClass(cls):
        cls.runModule("db.connect", flags="c")

    def test_dbcols_plain(self):
        cols = read_command("db.columns", table=self.invect, database=self.mapset)
        self.assertEqual(first=cols, second=output_plain)

        cols = read_command(
            "db.columns", table=self.invect, database=self.mapset, format="plain"
        )
        self.assertEqual(first=cols, second=output_plain)

    def test_dbcols_json(self):
        cols = parse_command(
            "db.columns", table=self.invect, database=self.mapset, format="json"
        )
        self.assertEqual(first=cols, second=output_json)

    def test_dbcols_json_more_info(self):
        cols = parse_command(
            "db.columns",
            table=self.invect,
            database=self.mapset,
            flags="e",
            format="json",
        )
        self.assertEqual(first=cols, second=output_json_more_info)

    def test_dbcols_plain_more_info(self):
        cols = read_command(
            "db.columns", table=self.invect, database=self.mapset, flags="e"
        )
        self.assertEqual(first=cols, second=output_plain_more_info)

        cols = read_command(
            "db.columns",
            table=self.invect,
            database=self.mapset,
            flags="e",
            format="plain",
        )
        self.assertEqual(first=cols, second=output_plain_more_info)


if __name__ == "__main__":
    test()
