"""
Name:       r.basins.fill
Purpose:    Tests r.basins.fill and its flags/options.
	
Author:     Sunveer Singh
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	    License (>=v2). Read the file COPYING that comes with GRASS
	    for details.
"""
import unittest
from grass.gunittest.case import TestCase

@unittest.skip("See #3822")
class TestRasterbasin(TestCase):
    celevation='elevation'
    tgeology='geology'
    output='basinsoutput'
    input="lakes"

    @classmethod
    def setUpClass(cls):
        seed = 500
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.tgeology, flags='p')
        cls.runModule('r.watershed', elevation='elevation', stream=cls.celevation, threshold='50000', overwrite=True)
        cls.runModule('r.geomorphon', elevation=cls.celevation, forms=cls.tgeology, overwrite=True)
        cls.runModule('r.mapcalc', seed=seed, expression='rand_cell = rand(1, 200)', overwrite=True)
        cls.runModule('r.thin', input=cls.input, output=cls.output)
        

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        
    def tearDown(cls):
        cls.runModule('g.remove', flags='f', type='raster', name=cls.output)

    def test_no1(self):
        lakes='lakes'
        self.assertModule('r.basins.fill', cnetwork=self.celevation, tnetwork=self.tgeology, output=self.output, number='1', overwrite=True)
        self.assertRasterMinMax(map=lakes, refmin=34300, refmax=43600,
	                        msg="lakes in degrees must be between 34300 and 43600")

    def test_no2(self):
        soils='soils'
        self.assertModule('r.basins.fill', cnetwork=self.celevation, tnetwork=self.tgeology, output=self.output, number='3')
        self.assertRasterMinMax(map=soils, refmin=18683, refmax=46555,
	                        msg="soils in degrees must be between 18683 and 46555")

    def test_no3(self):
        landuse='landuse'
        self.assertModule('r.basins.fill', cnetwork=self.celevation, tnetwork=self.tgeology, output=self.output, number='4')
        self.assertRasterMinMax(map=landuse, refmin=1, refmax=7,
	                        msg="landuse in degrees must be between 1 and 7")

    def test_no4(self):
        rand_cell='rand_cell'
        self.assertModule('r.basins.fill', cnetwork=self.celevation, tnetwork=self.tgeology, output=self.output, number='5')
        self.assertRasterMinMax(map=rand_cell, refmin=1, refmax=199,
	                        msg="rand_cell in degrees must be between 1 and 199")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
