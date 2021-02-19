# -*- coding: utf-8 -*-

import grass.lib.gis as libgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestGFatalError(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        libgis.G_fatal_error("Testing G_fatal_error() function call")


if __name__ == '__main__':
    test()
