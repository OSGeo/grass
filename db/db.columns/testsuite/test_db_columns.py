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

output_list_more_info = """cat INTEGER
OBJECTID INTEGER
WAKE_ZIPCO DOUBLE PRECISION
PERIMETER DOUBLE PRECISION
ZIPCODE_ DOUBLE PRECISION
ZIPCODE_ID DOUBLE PRECISION
ZIPNAME CHARACTER
ZIPNUM DOUBLE PRECISION
ZIPCODE CHARACTER
NAME CHARACTER
SHAPE_Leng DOUBLE PRECISION
SHAPE_Area DOUBLE PRECISION
"""

output_list_semicol_more_info = (
    "cat INTEGER;OBJECTID INTEGER;WAKE_ZIPCO DOUBLE PRECISION;"
    "PERIMETER DOUBLE PRECISION;ZIPCODE_ DOUBLE PRECISION;"
    "ZIPCODE_ID DOUBLE PRECISION;ZIPNAME CHARACTER;ZIPNUM DOUBLE PRECISION;"
    "ZIPCODE CHARACTER;NAME CHARACTER;SHAPE_Leng DOUBLE PRECISION;"
    "SHAPE_Area DOUBLE PRECISION"
    "\n"
)

output_csv = """name
cat
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

output_csv_more_info = """name,sql_type,is_number
cat,INTEGER,true
OBJECTID,INTEGER,true
WAKE_ZIPCO,DOUBLE PRECISION,true
PERIMETER,DOUBLE PRECISION,true
ZIPCODE_,DOUBLE PRECISION,true
ZIPCODE_ID,DOUBLE PRECISION,true
ZIPNAME,CHARACTER,false
ZIPNUM,DOUBLE PRECISION,true
ZIPCODE,CHARACTER,false
NAME,CHARACTER,false
SHAPE_Leng,DOUBLE PRECISION,true
SHAPE_Area,DOUBLE PRECISION,true
"""

output_tsv = """name\tsql_type\tis_number
cat\tINTEGER\ttrue
OBJECTID\tINTEGER\ttrue
WAKE_ZIPCO\tDOUBLE PRECISION\ttrue
PERIMETER\tDOUBLE PRECISION\ttrue
ZIPCODE_\tDOUBLE PRECISION\ttrue
ZIPCODE_ID\tDOUBLE PRECISION\ttrue
ZIPNAME\tCHARACTER\tfalse
ZIPNUM\tDOUBLE PRECISION\ttrue
ZIPCODE\tCHARACTER\tfalse
NAME\tCHARACTER\tfalse
SHAPE_Leng\tDOUBLE PRECISION\ttrue
SHAPE_Area\tDOUBLE PRECISION\ttrue
"""


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

    def test_dbcols_csv(self):
        cols = read_command(
            "db.columns", table=self.invect, database=self.mapset, format="csv"
        )
        self.assertEqual(cols, output_csv)

    def test_dbcols_list(self):
        cols = read_command(
            "db.columns", table=self.invect, database=self.mapset, format="list"
        )
        self.assertEqual(cols, output_plain)  # same as plain

    def test_dbcols_list_more_info(self):
        cols = read_command(
            "db.columns",
            table=self.invect,
            database=self.mapset,
            format="list",
            flags="-e",
        )
        self.assertEqual(cols, output_list_more_info)

        # with semicolumn as separator:
        cols = read_command(
            "db.columns",
            table=self.invect,
            database=self.mapset,
            format="list",
            flags="-e",
            separator=";",
        )
        self.assertEqual(cols, output_list_semicol_more_info)

    def test_dbcols_csv_more_info(self):
        cols = read_command(
            "db.columns",
            table=self.invect,
            database=self.mapset,
            format="csv",
            flags="-e",
        )
        self.assertEqual(cols, output_csv_more_info)

        # with tab as separator:
        cols = read_command(
            "db.columns",
            table=self.invect,
            database=self.mapset,
            format="csv",
            flags="-e",
            separator="\t",
        )
        self.assertEqual(cols, output_tsv)


if __name__ == "__main__":
    test()
