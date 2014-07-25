# -*- coding: utf-8 -*-

import os
from grass.gunittest import TestCase, test


class TestOsExit(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        os._exit(1)


if __name__ == '__main__':
    test()
