from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command, parse_command


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
        actual_csv = read_command(
            "v.db.connect", map="bridges", flags="p", format="csv"
        )
        self.assertEqual(actual_csv, actual)

    def test_json_output(self):
        """Test JSON output fields and values"""
        actual = parse_command("v.db.connect", map="bridges", flags="p", format="json")
        self.assertEqual(len(actual), 1)
        self.assertEqual(actual[0]["driver"], "sqlite")
        self.assertEqual(actual[0]["key"], "cat")
        self.assertEqual(actual[0]["layer"], 1)
        self.assertEqual(actual[0]["name"], "bridges")
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

        # Repeat check using explicit plain format
        actual = read_command(
            "v.db.connect", map="bridges", flags="c", format="plain"
        ).splitlines()
        self.assertEqual(actual, expected)

        # Repeat check using explicit CSV format
        actual = read_command(
            "v.db.connect", map="bridges", flags="c", format="csv"
        ).splitlines()
        self.assertEqual(actual, expected)

    def test_columns_json(self):
        """Test -c flag with JSON format"""
        actual = parse_command("v.db.connect", map="bridges", flags="c", format="json")
        expected = [
            {"name": "cat", "type": "INTEGER"},
            {"name": "OBJECTID", "type": "INTEGER"},
            {"name": "BRIDGES__1", "type": "DOUBLE PRECISION"},
            {"name": "SIPS_ID", "type": "CHARACTER"},
            {"name": "TYPE", "type": "CHARACTER"},
            {"name": "CLASSIFICA", "type": "CHARACTER"},
            {"name": "BRIDGE_NUM", "type": "DOUBLE PRECISION"},
            {"name": "FEATURE_IN", "type": "CHARACTER"},
            {"name": "FACILITY_C", "type": "CHARACTER"},
            {"name": "LOCATION", "type": "CHARACTER"},
            {"name": "YEAR_BUILT", "type": "DOUBLE PRECISION"},
            {"name": "WIDTH", "type": "DOUBLE PRECISION"},
            {"name": "CO_", "type": "DOUBLE PRECISION"},
            {"name": "CO_NAME", "type": "CHARACTER"},
        ]
        self.assertEqual(actual, expected)


if __name__ == "__main__":
    test()
