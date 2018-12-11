"""
Name:       i.segment test
Purpose:    Tests i.segment and its flags/options.
    
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
        map_input = 'ortho_2001_t792_1m'
        cls.runModule("g.region", n=220000, s=219456, w=637033, e=638000, align=map_input)
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        map_output = 'test'
        group = 'ortho'
        cls.runModule("g.remove", flags='f', type='raster', name=map_output)
        cls.runModule("g.remove", flags='f', type='group', name=group)
        cls.del_temp_region()

    def test_isegment(self):
        """Testing i.segment"""
        map_input = 'ortho_2001_t792_1m'
        map_output = 'test'
        group = 'ortho'

        self.assertModule('i.group', group=group, input=map_input)
        self.assertModule('i.segment', group=group, threshold=0.01, minsize=1, output=map_output)
        self.assertRasterMinMax(map=map_output, refmin=1, refmax=500000,
                                msg="Number of segments must be > 0")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
