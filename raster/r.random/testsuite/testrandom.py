"""
Name:       r.random test
Purpose:    Tests r.to.vect and its flags/options.

Author:     Sunveer Singh, Google Code-in 2018
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
                License (>=v2). Read the file COPYING that comes with GRASS
                for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

class Testrr(TestCase):

    input='lakes'
    cover="elevation"
    raster="routfile"
    vector="voutfile"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input) 

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
    
    def tearDown(self):
        """Remove the vector map after each test method"""

        self.runModule('g.remove', flags='f', type='vector', name=self.vector)
        self.runModule('g.remove', flags='f', type='raster', name=self.raster)

    def test_flag_z(self):
        """Testing flag z"""
        string="""area_cat|count|sum
        1|0|null
        2|0|null
        3|0|null
        4|0|null
        5|0|null
        6|0|null
        7|0|null
        8|0|null
        9|1|7
        10|0|null
        """
        r_random = SimpleModule('r.random', input=self.input, cover=self.cover, npoints=100, vector=self.vector, flags='z')
        r_random.outputs.stdout= string
        self.assertLooksLike(reference=string, actual=r_random.outputs.stdout)

    def test_flag_i(self):
        """Testing flag i"""
        self.assertModule('r.random', input=self.input, cover=self.cover, npoints=100, flags='i')

    def test_flag_d(self):
        """Testing flag d"""
        self.assertModule('r.random', input=self.input, cover=self.cover, npoints=100, vector=self.vector, flags='d')
        self.assertModule('v.info', map=self.vector, flags='t')
        topology = dict(points=100, lines=0, areas=0, map3d=1)
        self.assertVectorFitsTopoInfo(self.vector, topology)  

    def test_flag_b(self):
        """Testing flag b"""
        self.assertModule('r.random', input=self.input, cover=self.cover,
                          npoints=36011, vector=self.vector, flags='b',
                          overwrite=True)
        self.assertModule('v.info', map=self.vector, flags='t') 
        topology = dict(points=36011, lines=0, areas=0)
        self.assertVectorFitsTopoInfo(self.vector, topology) 
        


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
