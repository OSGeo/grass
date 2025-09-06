from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command, parse_command

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


if __name__ == "__main__":
    test()
