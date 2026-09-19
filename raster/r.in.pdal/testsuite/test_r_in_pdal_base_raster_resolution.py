"""
Name:      r.in.pdal base raster resolution flag test
Purpose:   Validates that -d flag preserves computational region resolution

Author:    Saurabh Sharma
Copyright: (C) 2026 by Saurabh Sharma and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import unittest
import shutil
from pathlib import Path
from tempfile import TemporaryDirectory

from grass.script import core as grass
from grass.script import raster as grast
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class BaseRasterResolutionTest(TestCase):
    """Test that -d flag preserves computational region resolution

    This test verifies the fix for issue #6740 where the -d flag was
    incorrectly changing the output raster resolution to match the base
    raster resolution instead of preserving the computational region resolution.

    The -d flag should only affect how base raster values are read (at base
    raster resolution), not the output raster resolution (which should match
    the computational region).

    This test requires pdal CLI util to be available.
    """

    @classmethod
    @unittest.skipIf(shutil.which("pdal") is None, "Cannot find pdal utility")
    def setUpClass(cls):
        """Set up test environment with different resolution regions"""
        cls.use_temp_region()
        # Set initial region
        cls.runModule("g.region", n=18, s=0, e=18, w=0, res=6)

        # Create test data directory
        cls.data_dir = os.path.join(Path(__file__).parent.absolute(), "data")
        cls.point_file = os.path.join(cls.data_dir, "points.csv")
        cls.tmp_dir = TemporaryDirectory()
        cls.las_file = os.path.join(cls.tmp_dir.name, "points.las")

        # Convert CSV to LAS
        grass.call(
            [
                "pdal",
                "translate",
                "-i",
                cls.point_file,
                "-o",
                cls.las_file,
                "-r",
                "text",
                "-w",
                "las",
                "--writers.las.format=0",
                "--writers.las.extra_dims=all",
                "--writers.las.minor_version=4",
            ]
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.tmp_dir.cleanup()
        cls.del_temp_region()

    def setUp(self):
        """Set up for each test - ensure clean region"""
        # Reset region to default for this test
        self.runModule("g.region", n=18, s=0, e=18, w=0, res=6)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def tearDown(self):
        """Remove the outputs created by the test"""
        # Reset region before cleanup
        self.runModule("g.region", n=18, s=0, e=18, w=0, res=6)
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            pattern="test_base_raster_*",
        )

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_base_raster_flag_preserves_region_resolution(self):
        """Test that -d flag preserves computational region resolution

        This is the main test for issue #6740. It verifies that when using
        the -d flag with a base raster of different resolution, the output
        raster resolution matches the computational region, not the base raster.
        """
        # Create a base raster at finer resolution (2m)
        # This simulates a high-resolution DEM
        self.runModule("g.region", n=18, s=0, e=18, w=0, res=2)
        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output="test_base_raster_fine",
            flags="o",
            quiet=True,
            method="mean",
        )

        # Set computational region to coarser resolution (6m)
        self.runModule("g.region", n=18, s=0, e=18, w=0, res=6)

        # Import with -d flag using the fine resolution base raster
        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output="test_base_raster_output",
            base_raster="test_base_raster_fine",
            flags="od",  # o=override projection, d=use base raster resolution
            quiet=True,
            method="mean",
        )

        # Check that output raster exists
        self.assertRasterExists("test_base_raster_output")

        # Get output raster info
        info = grast.raster_info("test_base_raster_output")

        # Verify that output resolution matches computational region (6m), not base raster (2m)
        self.assertAlmostEqual(
            info["nsres"],
            6.0,
            places=1,
            msg="Output north-south resolution should be 6m (computational region), not 2m (base raster)",
        )
        self.assertAlmostEqual(
            info["ewres"],
            6.0,
            places=1,
            msg="Output east-west resolution should be 6m (computational region), not 2m (base raster)",
        )

        # Verify that output has correct number of rows and columns for 6m resolution
        # Region is 18x18 with 6m resolution = 3x3 cells
        self.assertEqual(
            int(info["rows"]),
            3,
            "Output should have 3 rows (18m / 6m resolution)",
        )
        self.assertEqual(
            int(info["cols"]),
            3,
            "Output should have 3 columns (18m / 6m resolution)",
        )


if __name__ == "__main__":
    test()
