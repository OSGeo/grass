"""
Name:       r.geomorphon tests
Purpose:    Tests r.geomorphon input parsing.
            Uses NC Basic data set.

Author:     Luca Delucchi, Markus Neteler
Copyright:  (C) 2017 by Luca Delucchi, Markus Neteler and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command

synth_out = \
"""1	flat
3	ridge
4	shoulder
6	slope
8	footslope
9	valley
"""

ele_out = \
"""1	flat
2	summit
3	ridge
4	shoulder
5	spur
6	slope
7	hollow
8	footslope
9	valley
10	depression
"""

class TestClipling(TestCase):
    inele = 'elevation'
    insint = 'synthetic_dem'
    outele = 'ele_geomorph'
    outsint = 'synth_geomorph'

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.inele)
        cls.runModule('r.mapcalc', expression="{ou} = sin(x() / 5.0) + (sin(x() / 5.0) * 100.0 + 200)".format(ou=cls.insint))

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=(cls.insint, cls.outele, cls.outsint))
        cls.del_temp_region()
    
    def test_ele(self):
        self.runModule('r.geomorphon', elevation=self.inele, forms=self.outele,
                      search=10)
        category = read_command('r.category', map=self.outele)
        self.assertEqual(first=ele_out, second=category)

    def test_sint(self):
        self.runModule('r.geomorphon', elevation=self.insint,
                       forms=self.outsint, search=10)
        category = read_command('r.category', map=self.outsint)
        self.assertEqual(first=synth_out, second=category)
    
if __name__ == '__main__':
    test()
