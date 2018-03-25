"""
Name:       r.mode test
Purpose:    Tests r.mode and its flags/options.
	
Author:     Supreet Singh
Copyright:  (C) 2018 by Supreet Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.

"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

class Testrmode(TestCase):
    output='rmode'
    base='geology'
    cover='soils'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.base, flags='d')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule('g.remove', type='raster', flags='f', name=self.output)

    def test_1(self):
        soilsID='soils'
        self.assertModule('r.mode', base=self.base, cover=self.cover, output=self.output)
        self.assertRasterMinMax(map=soilsID, refmin=18683, refmax=46555,
	                        msg="soilsID in degrees must be between 18683 and 46555")

    def test_2(self):
        lakes='lakes'
        self.assertModule('r.mode', base=self.base, cover=self.cover, output=self.output)
        self.assertRasterMinMax(map=lakes, refmin=34300, refmax=43600,
	                        msg="lakes in degrees must be between 34300 and 43600")

    def test_3(self):
        elevation='elevation'
        self.assertModule('r.mode', base=self.base, cover=self.cover, output=self.output)
        self.assertRasterMinMax(map=elevation, refmin=2, refmax=4,
	                        msg="elevation in degrees must be between NULL and NULL")

if __name__ == '__main__':
    test()

