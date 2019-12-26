"""
Name:       r.mapcalc.simple test
Purpose:    Tests r.mapcalc.simple and its flags/options.
    
Author:     Markus Neteler
Copyright:  (C) 2018 by Markus Neteler and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""
from grass.gunittest.case import TestCase

class TestReport(TestCase):

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        map_input = 'elevation'
        cls.runModule("g.region", raster=map_input)
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        map_output1 = 'test1'
        map_output2 = 'test2'
        cls.runModule("g.remove", flags='f', type='raster', name=map_output1)
        cls.runModule("g.remove", flags='f', type='raster', name=map_output2)
        cls.del_temp_region()

    def test_rmapcalcsimple(self):
        """Testing r.mapcalc.simple"""
        map_input = 'elevation'
        map_output1 = 'test1'
        map_output2 = 'test2'

        # test 1
        self.assertModule('r.mapcalc.simple', expression='0', output=map_output1)
        self.assertRasterMinMax(map=map_output1, refmin=0, refmax=0,
                                msg="Result must be 0 for all pixels")

        # test 2
        formula='if(%s > 2000, 1, 0)' % map_input # expected to be 0
        self.assertModule('r.mapcalc.simple', expression=formula, output=map_output2)
        self.assertRasterMinMax(map=map_output2, refmin=0, refmax=0,
                                msg="Result must be 0 for all pixels")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
