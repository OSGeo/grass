# -*- coding: utf-8 -*-

import sys
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestSysExit(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        sys.exit(1)


if __name__ == '__main__':
    test()
