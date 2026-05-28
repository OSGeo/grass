from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import parse_command, read_command


class TestDbConnect(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runModule("db.connect", flags="d")

    def test_db_connect_plain(self):
        """Test plain text output"""
        actual = read_command("db.connect", flags="p").splitlines()
        self.assertEqual(len(actual), 4)
        self.assertEqual(actual[0], "driver: sqlite")
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(actual[1], r"database: .+sqlite\.db")
        self.assertEqual(actual[2], "schema: ")
        self.assertEqual(actual[3], "group: ")

        # Repeat with explicit plain format
        actual_plain = read_command(
            "db.connect", flags="p", format="plain"
        ).splitlines()
        self.assertEqual(actual, actual_plain)

    def test_db_connect_shell(self):
        """Test shell-format output"""
        actual = read_command("db.connect", flags="g").splitlines()
        self.assertEqual(len(actual), 4)
        self.assertEqual(actual[0], "driver=sqlite")
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(actual[1], r"database=.+sqlite\.db")
        self.assertEqual(actual[2], "schema=")
        self.assertEqual(actual[3], "group=")

        # Repeat with explicit shell format
        actual_shell = read_command(
            "db.connect", flags="p", format="shell"
        ).splitlines()
        self.assertEqual(actual, actual_shell)

    def test_db_connect_json(self):
        """Test JSON output"""
        expected = {
            "driver": "sqlite",
            "database_template": "$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db",
            "database": r".+sqlite\.db",
            "schema": None,
            "group": None,
        }
        actual = parse_command("db.connect", flags="p", format="json")
        self.assertEqual(list(actual.keys()), list(expected.keys()))
        self.assertEqual(actual["driver"], expected["driver"])
        self.assertEqual(actual["database_template"], expected["database_template"])
        # Since the database path is system-dependent, we verify it using a regular expression
        self.assertRegex(actual["database"], expected["database"])
        self.assertEqual(actual["schema"], expected["schema"])
        self.assertEqual(actual["group"], expected["group"])


if __name__ == "__main__":
    test()
