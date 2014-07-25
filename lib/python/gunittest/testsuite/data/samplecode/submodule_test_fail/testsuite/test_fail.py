# -*- coding: utf-8 -*-

from grass.gunittest import TestCase, test


class TestFail(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        self.assertTrue(False)


if __name__ == '__main__':
    test()
