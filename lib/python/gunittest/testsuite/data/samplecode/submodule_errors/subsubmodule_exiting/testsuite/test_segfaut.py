# -*- coding: utf-8 -*-

import ctypes
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestSegfault(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        """Crash the Python interpreter"""
        i = ctypes.c_char('a')
        j = ctypes.pointer(i)
        c = 0
        while True:
                j[c] = 'a'
                c += 1
        j


if __name__ == '__main__':
    test()
