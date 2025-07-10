from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command


class TestDbConnect(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runModule("db.connect", flags="c")

    def test_db_connect_plain(self):
        """Test plain text output"""
        result = read_command("db.connect", flags="p")
        self.assertIn("driver: ", result)
        self.assertIn("database: ", result)
        self.assertIn("schema: ", result)
        self.assertIn("group: ", result)

    def test_db_connect_shell(self):
        """Test shell-format output"""
        result = read_command("db.connect", flags="g")
        self.assertIn("driver=", result)
        self.assertIn("database=", result)
        self.assertIn("schema=", result)
        self.assertIn("group=", result)


if __name__ == "__main__":
    test()
