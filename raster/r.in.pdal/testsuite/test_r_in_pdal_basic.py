"""
Name:      r.in.pdal basic test
Purpose:   Test of basic r.in.pdal functionality

Author:    Vaclav Petras
Copyright: (C) 2019 by Vaclav Petras and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class BasicTest(TestCase):
    """Test case for watershed module

    This tests expects v.random and v.out.lidar to work properly.
    """

    # Setup variables to be used for outputs
    vector_points = 'rinpdal_basic_original'
    basic_raster = 'rinpdal_basic_raster'
    las_file = 'rinpdal_basic_points.las'
    npoints = 300

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule('g.region', n=20, s=10, e=25, w=15, res=1)
        cls.runModule('v.random', flags='zb', output=cls.vector_points,
            npoints=cls.npoints, zmin=200, zmax=500, seed=100)
        cls.runModule('v.out.lidar', input=cls.vector_points,
            output=cls.las_file)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule('g.remove', flags='f', type='vector',
            name=cls.vector_points)
        if os.path.isfile(cls.las_file):
            os.remove(cls.las_file)
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created by the import

        This is executed after each test run.
        """
        self.runModule('g.remove', flags='f', type='raster',
            name=self.basic_raster)

    def test_method_count(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('r.in.pdal', input=self.las_file,
            output=self.basic_raster, flags='eo', method="n")
        self.assertRasterExists(self.basic_raster)
        self.assertRasterMinMax(
            map=self.basic_raster,
            refmin=0,
            refmax=self.npoints)

    def test_method_mean(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('r.in.pdal', input=self.las_file,
            output=self.basic_raster, flags='eo', method="mean")
        self.assertRasterExists(self.basic_raster)
        self.assertRasterMinMax(
            map=self.basic_raster,
            refmin=200,
            refmax=500)


if __name__ == '__main__':
    test()
