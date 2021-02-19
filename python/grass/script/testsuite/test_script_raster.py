# -*- coding: utf-8 -*-
"""
Created on Thu Feb 18 09:42:23 2016

@author: lucadelu
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.script as gs


class TestRaster(TestCase):
    """Test raster functions"""

    raster = 'testrasterscript'
    region = gs.region()
    coords = (region['e'] - 1, region['n'] - 1)

    @classmethod
    def setUpClass(cls):
        cls.runModule("r.mapcalc", expression="testrasterscript = 100",
                      overwrite=True)

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", type='raster', name='testrasterscript',
                      flags='f')

    def test_raster_what(self):
        res = gs.raster_what(self.raster, [self.coords])[0]
        self.assertEquals(int(res[self.raster]['value']), 100)

        res = gs.raster_what(self.raster, [self.coords], localized=True)[0]
        self.assertEquals(int(res[self.raster][_('value')]), 100)

    def test_raster_info(self):
        res = gs.raster_info(self.raster)
        self.assertEquals(str(res['cols']), str(self.region['cols']))
        self.assertEquals(str(res['north']), str(self.region['n']))

if __name__ == '__main__':
    test()
