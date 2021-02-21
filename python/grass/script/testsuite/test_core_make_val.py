# -*- coding: utf-8 -*-

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import _make_val


class TestCoreMakeVal(TestCase):
    """Tests function `_make_val` that convert value to unicode."""

    def test_bytes(self):
        self.assertEqual("text", _make_val(b"text"))

    def test_unicode(self):
        self.assertEqual("text", _make_val("text"))

    def test_int(self):
        self.assertEqual("123", _make_val(123))

    def test_float(self):
        self.assertEqual("1.23", _make_val(1.23))

    def test_iterable(self):
        test = "text", "text", 123, 1.23
        solution = "text,text,123,1.23"
        self.assertEqual(solution, _make_val(test))


if __name__ == "__main__":
    test()
