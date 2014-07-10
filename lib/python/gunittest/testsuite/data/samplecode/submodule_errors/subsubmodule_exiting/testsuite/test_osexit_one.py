# -*- coding: utf-8 -*-

import os

# TODO: change to GrassTestCase
from unittest import TestCase


class TestOsExit(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        os._exit(1)


if __name__ == '__main__':
    import unittest
    unittest.main()
