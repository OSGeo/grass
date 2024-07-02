from grass.exceptions import ScriptError
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestTextAssertions(TestCase):
    def test_get_value(self):
        error = ScriptError("error")
        self.assertEqual("error", error.value)

    def test_str(self):
        error = ScriptError("error")
        self.assertEqual("error", str(error))


if __name__ == "__main__":
    test()
