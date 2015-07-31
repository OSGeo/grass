# -*- coding: utf-8 -*-
"""
Created on Thu Jul 30 18:27:22 2015

@author: lucadelu
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from numpy.random import random
from grass.pygrass.raster import raster2numpy, numpy2raster, RasterRow


def check_raster(name):
    r = RasterRow(name)
    try:
        r.open(mode='r')
        r.close()
        return True
    except:
        return False


class NumpyTestCase(TestCase):

    @classmethod
    def setUpClass(cls):
        """Create a not empty table instance"""

        cls.name = 'elevation'
        cls.tmp = 'tmp' + cls.name
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.name)
        cls.numpy_obj = raster2numpy(cls.name)

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exist"""
        from grass.pygrass.modules.shortcuts import general as g
        g.remove(type='raster', name=cls.tmp, flags='f')
        cls.del_temp_region()

    def test_type(self):
        self.assertTrue(str(self.numpy_obj.dtype), 'float32')

    def test_len(self):
        self.assertTrue(len(self.numpy_obj), 1350)
        self.assertTrue(len(self.numpy_obj[0]), 1500)

    def test_write(self):
        ran = random([1350, 1500])
        numpy2raster(ran, 'FCELL', self.tmp, True)
        self.assertTrue(check_raster(self.tmp))

if __name__ == '__main__':
    test()
