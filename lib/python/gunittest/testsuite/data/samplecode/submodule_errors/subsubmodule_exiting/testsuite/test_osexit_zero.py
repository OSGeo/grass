# -*- coding: utf-8 -*-

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestOsExit(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        os._exit(0)


if __name__ == '__main__':
    test()
