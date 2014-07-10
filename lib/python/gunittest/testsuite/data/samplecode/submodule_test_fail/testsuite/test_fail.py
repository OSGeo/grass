# -*- coding: utf-8 -*-

# TODO: change to GrassTestCase
from unittest import TestCase


class TestFail(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        self.assertTrue(False)


if __name__ == '__main__':
    import unittest
    unittest.main()
