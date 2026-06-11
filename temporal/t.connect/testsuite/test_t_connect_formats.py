"""
Test output formats for t.connect (plain, shell and json)
"""

import json

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestTConnectFormat(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initialize a default connection to ensure there is data to print."""
        cls.runModule(
            "t.connect",
            driver="sqlite",
            database="$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db",
        )

    def test_plain_format_values(self):
        """Test default plain text output contains the exact values."""
        t_conn = SimpleModule("t.connect", flags="p")
        self.assertModule(t_conn)
        out = t_conn.outputs.stdout

        self.assertIn("driver: sqlite", out)
        self.assertIn("database: $GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db", out)

    def test_shell_format_values(self):
        """Test shell format outputs exact keys and values with '=' delimiter."""
        t_conn = SimpleModule("t.connect", flags="p", format="shell")
        self.assertModule(t_conn)
        out = t_conn.outputs.stdout

        self.assertIn("driver=sqlite", out)
        self.assertIn("database=$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db", out)

    def test_json_format_values(self):
        """Test JSON output parses correctly and contains the exact values."""
        t_conn = SimpleModule("t.connect", flags="p", format="json")
        self.assertModule(t_conn)
        out = t_conn.outputs.stdout

        try:
            data = json.loads(out)
        except ValueError:
            self.fail("Output is not valid JSON")

        self.assertEqual(data.get("driver"), "sqlite")
        self.assertEqual(
            data.get("database"), "$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db"
        )

    def test_incompatible_flags(self):
        """Ensure using -g with format=json triggers a fatal error."""
        t_conn = SimpleModule("t.connect", flags="g", format="json")
        self.assertModuleFail(t_conn)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
