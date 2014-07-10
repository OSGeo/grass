# -*- coding: utf-8 -*-

import sys

# TODO: change to GrassTestCase
from unittest import TestCase


class TestSysExit(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        sys.exit(1)


if __name__ == '__main__':
    import unittest
    unittest.main()
