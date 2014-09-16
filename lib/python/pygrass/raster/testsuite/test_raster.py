# -*- coding: utf-8 -*-
"""
Created on Mon Sep 15 13:37:24 2014

@author: lucadelu
"""

from grass.gunittest import TestCase, test

from grass.pygrass.raster import RasterRow


class RasterRowTestCate(TestCase):

    name = 'elevation'

    def test_type(self):
        eletype = 'FCELL'
        r = RasterRow(self.name)
        r.open()
        self.assertTrue(r.mtype, eletype)
        r.close()

    def test_isopen(self):
        r = RasterRow(self.name)
        self.assertFalse(r.is_open())
        r.open()
        self.assertTrue(r.is_open())
        r.close()

    def test_name(self):
        r = RasterRow(self.name)
        r.open()
        self.assertEqual(r.name, self.name)
        fullname = "{name}@{mapset}".format(name=r.name, mapset=r.mapset)
        self.assertEqual(r.fullname(), fullname)
        r.close()


if __name__ == '__main__':
    test()
