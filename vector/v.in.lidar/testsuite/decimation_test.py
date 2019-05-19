"""
Name:      decimation_test
Purpose:   v.in.lidar decimation test

Author:    Vaclav Petras
Copyright: (C) 2015 by Vaclav Petras and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestCountBasedDecimation(TestCase):
    """Test case for watershed module

    This tests expects v.random and v.out.lidar to work properly.
    """

    # Setup variables to be used for outputs
    vector_points = 'vinlidar_decimation_original'
    imported_points = 'vinlidar_decimation_imported'
    las_file = 'vinlidar_decimation_points.las'
    npoints = 300  # the values works well for 300 without rounding

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
        self.runModule('g.remove', flags='f', type='vector',
            name=self.imported_points)

    def test_identical(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt')
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=self.npoints))

    def skip_number(self, number, expect):
        """Test to see if the outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt', skip=number)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=expect))

    def preserve_number(self, number, expect):
        """Test to see if the outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt', preserve=number)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=expect))

    def offset_number(self, number, expect):
        """Test to see if the outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt', offset=number)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=expect))

    def limit_number(self, number, expect):
        """Test to see if the outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt', limit=number)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=expect))

    def test_decimated_skip_2(self):
        """Test to see if the outputs are created"""
        self.skip_number(number=2, expect=self.npoints / 2)

    def test_decimated_skip_4(self):
        """Test to see if the outputs are created"""
        self.skip_number(number=4, expect=0.75 * self.npoints)

    def test_decimated_skip_10(self):
        """Test to see if the outputs are created"""
        self.skip_number(number=10, expect=0.9 * self.npoints)

    def test_decimated_preserve_2(self):
        """Test to see if the outputs are created"""
        self.preserve_number(number=2, expect=self.npoints / 2)

    def test_decimated_preserve_10(self):
        """Test to see if the outputs are created"""
        self.preserve_number(number=10, expect=self.npoints / 10)

    def test_decimated_offset_105(self):
        """Test to see if the outputs are created"""
        self.offset_number(number=105, expect=self.npoints - 105)

    def test_decimated_limit_105(self):
        """Test to see if the outputs are created"""
        self.limit_number(number=105, expect=105)

    def test_offset_preserve(self):
        """Test to see if the outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt',
            offset=105, preserve=10)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=int((self.npoints - 105) / 10)))

    def test_limit_skip(self):
        """Test to see if the outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt',
            limit=105, skip=10)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=105))

    def test_offset_limit_skip(self):
        """Test to see if the outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt',
            offset=50, skip=5, limit=self.npoints - 1)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=0.8 * (self.npoints - 50)))


if __name__ == '__main__':
    test()
