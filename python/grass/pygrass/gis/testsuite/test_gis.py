# -*- coding: utf-8 -*-
"""
Luca Delucchi
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.gis.region import Region


class RegionTestCase(TestCase):

    def test_bounds(self):
        reg1 = Region()
        reg2 = Region()
        self.assertTrue(reg1, reg2)
        north = reg2.north
        reg2.north = 0
        self.assertNotEqual(reg1.north, reg2.north)
        reg2.north = north


if __name__ == '__main__':
    test()
