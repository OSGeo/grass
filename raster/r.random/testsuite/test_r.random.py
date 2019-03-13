"""
Name:        r.random test
Purpose:    Tests r.random module and some of its options.

Author:     Shubham Sharma, Google Code-in 2018
Copyright:  (C) 2018 by Shubham Sharma and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRasterTile(TestCase):
    input = 'landcover_1m'
    npoints = '20'
    raster = 'landcover_1m_raster_random'
    vector = "landcover_1m_vector_random"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

        cls.runModule('g.remove', type='raster', flags='f', name=cls.raster)
        cls.runModule('g.remove', type='raster', flags='f', name=cls.raster + '_null')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.raster + '_without_topology')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.raster + '_3D')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.raster + '_cover_landcover_1m')

        cls.runModule('g.remove', type='vector', flags='f', name=cls.vector)
        cls.runModule('g.remove', type='vector', flags='f', name=cls.vector+'_null')
        cls.runModule('g.remove', type='vector', flags='f', name=cls.vector + '_without_topology')
        cls.runModule('g.remove', type='vector', flags='f', name=cls.vector + '_3D')
        cls.runModule('g.remove', type='vector', flags='f', name=cls.vector + '_cover_landcover_1m')

    def test_random_raster(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', input=self.input, npoints=self.npoints, raster=self.raster)
        # check if random raster was created
        self.assertRasterExists(self.raster, msg="landcover_1m_raster_random was not created")

    def test_random_vector(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', input=self.input, npoints=self.npoints, vector=self.vector)
        # check if random vector was created
        self.assertVectorExists(self.vector, msg="landcover_1m_vector_random was not created")
        topology = dict(points=20,primitives=20)
        self.assertVectorFitsTopoInfo(vector=self.vector, reference=topology)

    def test_random_raster_flag_z(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', flags='z', input=self.input, npoints=self.npoints, raster=self.raster+'_null')
        # check if random raster ( with NULL values )  was created
        self.assertRasterExists(self.raster, msg="landcover_1m_raster_random_null was not created")


    def test_vector_random_flag_z(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', flags='z', input=self.input, npoints=self.npoints, vector=self.vector+'_null')
        # check if random vector ( with NULL values ) was created
        self.assertVectorExists(self.vector+'_null', msg="landcover_1m_vector_random_null was not created")
        topology = dict(points=20,primitives=20)
        self.assertVectorFitsTopoInfo(vector=self.vector+'_null', reference=topology)

    def test_random_raster_flag_b(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', flags='b', input=self.input, npoints=self.npoints, raster=self.raster+'_without_topology')
        # check if random raster ( without topology )  was created
        self.assertRasterExists(self.raster, msg="landcover_1m_raster_random_without_topologywas not created")

    def test_vector_random_flag_b(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', flags='b', input=self.input, npoints=self.npoints, vector=self.vector+'_without_topology')
        # check if random vector ( without topology ) was created
        self.assertVectorExists(self.vector+'_without_topology', msg="landcover_1m_vector_random_without_topology was not created")
        topology = dict(points=20,primitives=20)
        self.assertVectorFitsTopoInfo(vector=self.vector+'_without_topology', reference=topology)

    def test_random_raster_flag_d(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', flags='d', input=self.input, npoints=self.npoints, raster=self.raster+'_3D')
        # check if random raster ( 3D points )  was created
        self.assertRasterExists(self.raster, msg="landcover_1m_raster_random_3D not created")

    def test_vector_random_flag_d(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random', flags='d', input=self.input, npoints=self.npoints, vector=self.vector+'_3D')
        # check if random vector ( 3D points ) was created
        self.assertVectorExists(self.vector+'_3D', msg="landcover_1m_vector_random_3D was not created")
        topology = dict(points=20,primitives=20)
        self.assertVectorFitsTopoInfo(vector=self.vector+'_3D', reference=topology)


    def test_random_raster_cover(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random',input='landuse96_28m', npoints=self.npoints, cover='landcover_1m', raster=self.raster+'_cover_landcover_1m')
        # check if random raster ( 3D points )  was created
        self.assertRasterExists(self.raster+'_cover_landcover_1m', msg="landcover_1m_raster_random_cover_landcover_1m was not created")

    def test_vector_random_flag_d(self):
        """Testing r.random  runs successfully"""
        self.assertModule('r.random',input='landuse96_28m', npoints=self.npoints, cover='landcover_1m', vector=self.vector+'_cover_landcover_1m')
        # check if random vector ( 3D points ) was created
        self.assertVectorExists(self.vector+'_cover_landcover_1m', msg="landcover_1m_vector_cover_landcover_1m was not created")
        topology = dict(points=20, primitives=20)
        self.assertVectorFitsTopoInfo(vector=self.vector+'_cover_landcover_1m', reference=topology)


if __name__ == '__main__':
    test()
