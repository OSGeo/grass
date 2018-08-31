"""
Name:      test_v_out_lidar
Purpose:   v.out.lidar export test

Author:    Vaclav Petras
Copyright: (C) 2015 by Vaclav Petras and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class BasicTest(TestCase):
    """Test case for watershed module

    This tests expects v.random to work properly.
    """

    # Setup variables to be used for outputs
    vector_points = 'v_out_lidar_original'
    imported_points = 'v_out_lidar_imported'
    las_file = 'v_out_lidar_points.las'

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule('g.region', n=20, s=10, e=25, w=15, res=1)
        cls.runModule('v.random', flags='zb', output=cls.vector_points,
            npoints=300, zmin=200, zmax=500, seed=100)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule('g.remove', flags='f', type='vector',
            name=cls.vector_points)
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created by the export

        This is executed after each test run.
        """
        if os.path.isfile(self.las_file):
            os.remove(self.las_file)
        self.runModule('g.remove', flags='f', type='vector',
            name=self.imported_points)

    def test_module_runs_output_created(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('v.out.lidar', input=self.vector_points,
            output=self.las_file)
        self.assertFileExists(self.las_file)

    def test_output_identical(self):
        """Test to see if the standard outputs are created

        This test depends on v.in.lidar working properly.
        """
        self.assertModule('v.out.lidar', input=self.vector_points,
            output=self.las_file)
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt')
        self.assertVectorExists(self.imported_points)
        self.assertVectorEqualsVector(
            actual=self.imported_points,
            reference=self.vector_points,
            digits=2, precision=.01)


if __name__ == '__main__':
    test()
