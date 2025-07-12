from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command, parse_command


class TestDbConnect(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runModule("db.connect", flags="c")

    def _assert_output(self, result):
        expected_keys = ["driver", "database", "schema", "group"]
        # Ensure the same count and that each line starts with the expected key
        self.assertEqual(len(result), len(expected_keys))
        for i, key in enumerate(expected_keys):
            self.assertIn(key, result[i])

    def test_db_connect_plain(self):
        """Test plain text output"""
        actual = read_command("db.connect", flags="p").splitlines()
        self._assert_output(actual)

        # Repeat with explicit plain format
        actual = read_command("db.connect", flags="p", format="plain").splitlines()
        self._assert_output(actual)

    def test_db_connect_shell(self):
        """Test shell-format output"""
        actual = read_command("db.connect", flags="g").splitlines()
        self._assert_output(actual)

        # Repeat with explicit shell format
        actual = read_command("db.connect", flags="p", format="shell").splitlines()
        self._assert_output(actual)

    def test_db_connect_json(self):
        """Test JSON output"""
        result = parse_command("db.connect", flags="p", format="json")
        self._assert_output(list(result.keys()))


if __name__ == "__main__":
    test()
