# -*- coding: utf-8 -*-

import ctypes

# TODO: change to GrassTestCase
from unittest import TestCase


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
    import unittest
    unittest.main()
