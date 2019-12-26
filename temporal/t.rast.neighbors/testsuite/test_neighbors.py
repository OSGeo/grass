# -*- coding: utf-8 -*-
"""
Created on Wed Feb 17 19:53:04 2016

@author: lucadelu
"""

import os
import sys
import unittest
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

@unittest.skipIf(sys.version_info[0] > 2, "temporary disabled")
class TestAggregationAbsolute(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,
                      t=50,  res=10,  res3=10)
        cls.runModule("r.mapcalc", expression="a1 = rand(1,10)", flags=['s'],
                      overwrite=True)
        cls.runModule("r.mapcalc", expression="a2 = rand(1,10)", flags=['s'],
                      overwrite=True)
        cls.runModule("r.mapcalc", expression="a3 = rand(1,10)", flags=['s'],
                      overwrite=True)

        cls.runModule("t.create", type="strds", temporaltype="absolute",
                      output="A", title="A test", description="A test",
                      overwrite=True)

        cls.runModule("t.register", flags="i",  type="raster",  input="A",
                      maps="a1,a2,a3", start="2001-01-01 00:00:00",
                      increment="1 month", overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", type="strds", inputs="A")

    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove", flags="rf", type="strds", inputs="B")

    def test_where(self):
        """Test simple t.rast.neighbors"""
        trast_list = SimpleModule("t.rast.neighbors", quiet=True, input="A",
                                  output="B", basename='b', overwrite=True,
                                  size="5", where="start_time <= '2001-02-01 00:00:00'")
        self.assertModule(trast_list)
        self.assertRasterExists('b_2001_01')
        self.assertRasterMinMax('b_2001_02', 1, 10)
        self.assertRasterDoesNotExist('b_2001_03')

    def test_simple(self):
        """Test simple t.rast.neighbors"""
        trast_list = SimpleModule("t.rast.neighbors", quiet=True, input="A",
                                  output="B", size="5", basename='b',
                                  overwrite=True)
        self.assertModule(trast_list)
        self.assertRasterMinMax('b_2001_01', 1, 10)
        self.assertRasterMinMax('b_2001_02', 1, 10)

    def test_time_suffix(self):
        """Test simple t.rast.neighbors"""
        trast_list = SimpleModule("t.rast.neighbors", quiet=True, input="A",
                                  output="B", size="5", basename='b',
                                  suffix="time", overwrite=True)
        self.assertModule(trast_list)
        self.assertRasterExists('b_2001_01_01T00_00_00')

    def test_num_suffix(self):
        """Test simple t.rast.neighbors"""
        trast_list = SimpleModule("t.rast.neighbors", quiet=True, input="A",
                                  output="B", size="5", basename='b',
                                  suffix="num%03", overwrite=True)
        self.assertModule(trast_list)
        self.assertRasterExists('b_001')

    def test_num_region(self):
        """Test t.rast.neighbors with parallel region processes"""
        trast_list = SimpleModule("t.rast.neighbors", quiet=True, input="A",
                                  output="B", size="5", basename='b', nprocs=2,
                                  suffix="num%03", flags="r", overwrite=True)
        self.assertModule(trast_list)
        self.assertRasterExists('b_001')
        self.assertRasterExists('b_002')
        self.assertRasterExists('b_003')

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
