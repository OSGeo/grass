"""
Name:      basic_test
Purpose:   v.in.pdal basic functionality test

Author:    Vaclav Petras
Copyright: (C) 2015 by Vaclav Petras and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import unittest
from grass.script import shutil_which


class BasicTest(TestCase):
    """Test case for watershed module

    This tests expects v.random and v.out.lidar to work properly.
    """

    # Setup variables to be used for outputs
    vector_generated = "vinlidar_basic_generated"
    vector_points = "vinlidar_basic_original"
    imported_points = "vinlidar_basic_imported"
    las_file = "vinlidar_basic_points.las"
    npoints = 300

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", n=20, s=10, e=25, w=15, res=1)
        cls.runModule(
            "v.random",
            flags="zb",
            output=cls.vector_generated,
            npoints=cls.npoints,
            zmin=200,
            zmax=500,
            seed=100,
        )
        cls.runModule(
            "v.category",
            input=cls.vector_generated,
            output=cls.vector_points,
            option="del",
            cat=-1,
        )
        cls.runModule("v.out.lidar", input=cls.vector_points, output=cls.las_file)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="vector",
            name=(cls.vector_points, cls.vector_generated),
        )
        if os.path.isfile(cls.las_file):
            os.remove(cls.las_file)
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created by the import

        This is executed after each test run.
        """
        self.runModule("g.remove", flags="f", type="vector", name=self.imported_points)

    @unittest.skipIf(shutil_which("v.in.pdal") is None, "Cannot find v.in.pdal")
    def test_same_data(self):
        """Test to see if the standard outputs are created"""
        self.assertModule(
            "v.in.pdal", input=self.las_file, flags="c", output=self.imported_points
        )
        self.assertVectorExists(self.imported_points)
        self.assertVectorEqualsVector(
            actual=self.imported_points,
            reference=self.vector_points,
            digits=2,
            precision=0.01,
        )


if __name__ == "__main__":
    test()
