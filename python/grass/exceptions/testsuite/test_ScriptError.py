# -*- coding: utf-8 -*-

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.exceptions import ScriptError


class TestTextAssertions(TestCase):

    def test_get_value(self):
        error = ScriptError('error')
        self.assertEqual('error', error.value)

    def test_str(self):
        error = ScriptError('error')
        self.assertEqual('error', str(error))


if __name__ == '__main__':
    test()
