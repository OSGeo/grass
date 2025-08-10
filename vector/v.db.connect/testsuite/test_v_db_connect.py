from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command


class TestVDbConnect(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runModule("db.connect", flags="c")

    def test_plain_output(self):
        """Test default plain text output"""
        actual = read_command("v.db.connect", map="bridges", flags="p").splitlines()
        self.assertEqual(len(actual), 2)
        self.assertEqual(actual[0], "Vector map <bridges> is connected by:")
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(
            actual[1],
            r"layer <1\/bridges> table <bridges> in database <.+sqlite\.db> through driver <sqlite> with key <cat>",
        )

    def test_csv_output(self):
        """Test -g flag CSV output"""
        actual = read_command("v.db.connect", map="bridges", flags="g")
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(actual, r"1\/bridges\|bridges\|cat\|.+sqlite\.db\|sqlite")

    def test_columns_csv(self):
        """Test -c flag CSV output"""
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
        actual = read_command("v.db.connect", map="bridges", flags="c").splitlines()
        self.assertEqual(actual, expected)


if __name__ == "__main__":
    test()
