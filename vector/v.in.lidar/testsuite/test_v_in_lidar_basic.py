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


class BasicTest(TestCase):
    """Test case for watershed module

    This tests expects v.random and v.out.lidar to work properly.
    """

    # Setup variables to be used for outputs
    vector_points = 'vinlidar_basic_original'
    imported_points = 'vinlidar_basic_imported'
    las_file = 'vinlidar_basic_points.las'
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
        self.runModule('g.remove', flags='f', type='vector',
            name=self.imported_points)

    def test_output_identical(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt')
        self.assertVectorExists(self.imported_points)
        self.assertVectorEqualsVector(
            actual=self.imported_points,
            reference=self.vector_points,
            digits=2, precision=.01)


if __name__ == '__main__':
    test()
