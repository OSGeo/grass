# -*- coding: utf-8 -*-

import grass.lib.gis as libgis

# TODO: change to GrassTestCase
from unittest import TestCase


class TestGFatalError(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        libgis.G_fatal_error("Testing G_fatal_error() function call")


if __name__ == '__main__':
    import unittest
    unittest.main()
