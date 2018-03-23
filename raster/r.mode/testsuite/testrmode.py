"""
Name:       r.mode test
Purpose:    Tests r.mode and its flags/options.
    
Author:     Supreet Singh
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

class Testrmode(TestCase):
    output='rmode'
    base='facility'
    cover='soils_Kfactor'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', flags='d')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule('g.remove', type='raster', flags='f', name=self.output)

    def test_1(self):
        facility='facility'
        self.assertModule('r.mode', base=self.base, cover=self.cover,
                          output=self.output)
        self.assertRasterMinMax(map=facility, refmin=1, refmax=1,
                                msg="facility in degrees must be between "
                                    "1 and 1")

    def test_2(self):
        slope='slope'
        self.assertModule('r.mode', base=self.base, cover=self.cover,
                          output=self.output)
        self.assertRasterMinMax(map=slope, refmin=0, refmax=38.68939,
                                msg="slope in degrees must be between 0 and "
                                    "38.68939")

    def test_3(self):
        elevation='elevation'
        self.assertModule('r.mode', base=self.base, cover=self.cover,
                          output=self.output)
        self.assertRasterMinMax(map=elevation, refmin=55.57879,
                                refmax=156.3299, msg="elevation in degrees "
                                "must be between 55.57879 and 156.3299")

if __name__ == '__main__':
    test()

