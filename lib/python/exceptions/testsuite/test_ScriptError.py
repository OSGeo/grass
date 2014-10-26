# -*- coding: utf-8 -*-
from grass.gunittest import TestCase, test
from grass.exceptions import ScriptError


class TestTextAssertions(TestCase):

    def test_get_value(self):
        error = ScriptError('error')
        self.assertEqual('error', error.value)

    def test_str(self):
        error = ScriptError('error')
        self.assertEqual('error', str(error))
        error = ScriptError(12)
        self.assertEqual('12', str(error))


if __name__ == '__main__':
    test()
