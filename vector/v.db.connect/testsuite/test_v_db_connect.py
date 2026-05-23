import re

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command, parse_command
from grass.gunittest.gmodules import SimpleModule


class TestVDbConnect(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runModule("db.connect", flags="c")

    def test_plain_output(self):
        """Test default and explicit plain text outputs"""
        actual = read_command("v.db.connect", map="bridges", flags="p").splitlines()
        self.assertEqual(len(actual), 2)
        self.assertEqual(actual[0], "Vector map <bridges> is connected by:")
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(
            actual[1],
            r"layer <1\/bridges> table <bridges> in database <.+sqlite\.db> through driver <sqlite> with key <cat>",
        )

        # Repeat check using explicit plain format
        actual_plain = read_command(
            "v.db.connect", map="bridges", flags="p", format="plain"
        ).splitlines()
        self.assertEqual(actual_plain, actual)

    def test_csv_output(self):
        """Test -g flag CSV output"""
        actual = read_command("v.db.connect", map="bridges", flags="g")
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(actual, r"1\/bridges\|bridges\|cat\|.+sqlite\.db\|sqlite")

        # Repeat check using explicit CSV format
        actual = read_command(
            "v.db.connect", map="bridges", flags="p", format="csv"
        ).splitlines()
        self.assertEqual(len(actual), 2)
        self.assertEqual(actual[0], "layer,layer_name,table,key,database,driver")
        self.assertRegex(actual[1], r"1,bridges,bridges,cat,.+sqlite\.db,sqlite")

        # Repeat check using explicit CSV format and flag -g
        actual_csv = read_command(
            "v.db.connect", map="bridges", flags="g", format="csv"
        ).splitlines()
        self.assertEqual(actual_csv, actual)

    def test_json_output(self):
        """Test JSON output fields and values"""
        actual = parse_command("v.db.connect", map="bridges", flags="p", format="json")
        self.assertEqual(len(actual), 1)
        self.assertEqual(actual[0]["driver"], "sqlite")
        self.assertEqual(actual[0]["key"], "cat")
        self.assertEqual(actual[0]["layer"], 1)
        self.assertEqual(actual[0]["layer_name"], "bridges")
        self.assertEqual(actual[0]["table"], "bridges")
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(actual[0]["database"], r".+sqlite\.db")

    def test_columns_csv(self):
        """Test -c flag with CSV and plain formats"""
        expected = [
            "INTEGER|cat",
            "INTEGER|OBJECTID",
            "DOUBLE PRECISION|BRIDGES__1",
            "CHARACTER|SIPS_ID",
            "CHARACTER|TYPE",
            "CHARACTER|CLASSIFICA",
            "DOUBLE PRECISION|BRIDGE_NUM",
            "CHARACTER|FEATURE_IN",
            "CHARACTER|FACILITY_C",
            "CHARACTER|LOCATION",
            "DOUBLE PRECISION|YEAR_BUILT",
            "DOUBLE PRECISION|WIDTH",
            "DOUBLE PRECISION|CO_",
            "CHARACTER|CO_NAME",
        ]

        # Check using -c flag
        actual = read_command("v.db.connect", map="bridges", flags="c").splitlines()
        self.assertEqual(actual, expected)

        expected = [
            "name|sql_type",
            "cat|INTEGER",
            "OBJECTID|INTEGER",
            "BRIDGES__1|DOUBLE PRECISION",
            "SIPS_ID|CHARACTER",
            "TYPE|CHARACTER",
            "CLASSIFICA|CHARACTER",
            "BRIDGE_NUM|DOUBLE PRECISION",
            "FEATURE_IN|CHARACTER",
            "FACILITY_C|CHARACTER",
            "LOCATION|CHARACTER",
            "YEAR_BUILT|DOUBLE PRECISION",
            "WIDTH|DOUBLE PRECISION",
            "CO_|DOUBLE PRECISION",
            "CO_NAME|CHARACTER",
        ]

        # Repeat check using explicit plain format
        actual = read_command(
            "v.db.connect", map="bridges", flags="c", format="plain"
        ).splitlines()
        self.assertEqual(actual, expected)

        # Adjust expected for CSV format
        expected = [line.replace("|", ",") for line in expected]

        # Repeat check using explicit CSV format
        actual = read_command(
            "v.db.connect", map="bridges", flags="c", format="csv"
        ).splitlines()
        self.assertEqual(actual, expected)

    def _assert_no_layer_filename_warning(self, stderr):
        """Assert stderr does not contain the parser 'illegal filename' regression."""
        stderr = stderr or ""
        self.assertIsNone(
            re.search(r"illegal filename", stderr, re.IGNORECASE),
            msg=stderr,
        )
        self.assertIsNone(
            re.search(r"character\s*<\s*/\s*>\s*not allowed", stderr, re.IGNORECASE),
            msg=stderr,
        )

    def _columns_output(self, layer_arg=None):
        """Run v.db.connect -c and return stdout lines (column list)."""
        args = {"map": "bridges", "flags": "c"}
        if layer_arg is not None:
            args["layer"] = layer_arg
        m = SimpleModule("v.db.connect", **args)
        self.assertModule(m)
        self._assert_no_layer_filename_warning(m.outputs.stderr)
        return m.outputs.stdout.strip().splitlines()

    def test_layer_number_only(self):
        """layer=1 (number only) works and produces column list"""
        lines = self._columns_output(layer_arg="1")
        self.assertGreater(len(lines), 0)
        self.assertIn("INTEGER|cat", lines)

    def test_layer_number_slash_name(self):
        """layer=1/bridges (number/name) works like layer=1 with no warning"""
        ref_lines = self._columns_output(layer_arg="1")
        lines = self._columns_output(layer_arg="1/bridges")
        self.assertEqual(
            lines,
            ref_lines,
            "layer=1/bridges should produce same columns as layer=1",
        )

    # -p regression coverage: original warning was triggered during parsing for 1/name
    def test_layer_number_slash_name_print(self):
        """layer=1/bridges works with -p (print) without illegal-filename warning"""
        m = SimpleModule("v.db.connect", map="bridges", layer="1/bridges", flags="p")
        self.assertModule(m)
        self._assert_no_layer_filename_warning(m.outputs.stderr)

    def test_layer_name_only(self):
        """layer=bridges (name only) works and matches layer=1 output"""
        ref_lines = self._columns_output(layer_arg="1")
        lines = self._columns_output(layer_arg="bridges")
        self.assertEqual(lines, ref_lines)

    def test_columns_json(self):
        """Test -c flag with JSON format"""
        actual = parse_command("v.db.connect", map="bridges", flags="c", format="json")
        expected = [
            {"name": "cat", "sql_type": "INTEGER", "is_number": True},
            {"name": "OBJECTID", "sql_type": "INTEGER", "is_number": True},
            {"name": "BRIDGES__1", "sql_type": "DOUBLE PRECISION", "is_number": True},
            {"name": "SIPS_ID", "sql_type": "CHARACTER", "is_number": False},
            {"name": "TYPE", "sql_type": "CHARACTER", "is_number": False},
            {"name": "CLASSIFICA", "sql_type": "CHARACTER", "is_number": False},
            {"name": "BRIDGE_NUM", "sql_type": "DOUBLE PRECISION", "is_number": True},
            {"name": "FEATURE_IN", "sql_type": "CHARACTER", "is_number": False},
            {"name": "FACILITY_C", "sql_type": "CHARACTER", "is_number": False},
            {"name": "LOCATION", "sql_type": "CHARACTER", "is_number": False},
            {"name": "YEAR_BUILT", "sql_type": "DOUBLE PRECISION", "is_number": True},
            {"name": "WIDTH", "sql_type": "DOUBLE PRECISION", "is_number": True},
            {"name": "CO_", "sql_type": "DOUBLE PRECISION", "is_number": True},
            {"name": "CO_NAME", "sql_type": "CHARACTER", "is_number": False},
        ]
        self.assertEqual(actual, expected)


if __name__ == "__main__":
    test()
