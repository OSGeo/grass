"""
Name:      r_watershed_test
Purpose:   This script is to demonstrate a unit test for r.watershed
           module (originally developed for GIS582 course at NCSU).

Author:    Stephanie Wendel
Copyright: (C) 205 by Stephanie Wendel and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestWatershed(TestCase):
    """Test case for watershed module"""

    # Setup variables to be used for outputs
    accumulation = "test_accumulation"
    drainage = "test_drainage"
    basin = "test_basin"
    stream = "test_stream"
    halfbasin = "test_halfbasin"
    slopelength = "test_slopelength"
    slopesteepness = "test_slopesteepness"
    elevation = "elevation"
    lengthslope_2 = "test_lengthslope_2"
    stream_2 = "test_stream_2"
    tci = "test_tci"
    spi = "test_spi"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and setup"""
        # Always use the computational region of the raster elevation
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.elevation)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created from the watershed module

        This is executed after each test run.
        """
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            pattern="test_*",
        )

    def test_OutputCreated(self):
        """Test to see if the outputs are created"""
        # run the watershed module
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            accumulation=self.accumulation,
            drainage=self.drainage,
            basin=self.basin,
            stream=self.stream,
            half_basin=self.halfbasin,
            length_slope=self.slopelength,
            slope_steepness=self.slopesteepness,
        )
        # check to see if accumulation output is in mapset
        self.assertRasterExists(
            self.accumulation, msg="accumulation output was not created"
        )
        # check to see if drainage output is in mapset
        self.assertRasterExists(self.drainage, msg="drainage output was not created")
        # check to see if basin output is in mapset
        self.assertRasterExists(self.basin, msg="basin output was not created")
        # check to see if stream output is in mapset
        self.assertRasterExists(self.stream, msg="stream output was not created")
        # check to see if half.basin output is in mapset
        self.assertRasterExists(self.halfbasin, msg="half.basin output was not created")
        # check to see if length.slope output is in mapset
        self.assertRasterExists(
            self.slopelength, msg="length.slope output was not created"
        )
        # check to see if slope.steepness output is in mapset
        self.assertRasterExists(
            self.slopesteepness, msg="slope.steepness output was not created"
        )

    def test_fourFlag(self):
        """Test the -4 flag and the stream and slope lengths

        Tests the -4 flag to see if the stream and slope lengths are
        approximately the same as the outputs from the default
        module run.
        """
        # Run module with default settings
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            stream=self.stream,
            length_slope=self.slopelength,
            overwrite=True,
        )
        # Run module with flag 4
        self.assertModule(
            "r.watershed",
            flags="4",
            elevation="elevation",
            threshold="10000",
            stream=self.stream_2,
            length_slope=self.lengthslope_2,
        )
        # Use the assertRastersNoDifference with precision 100 to see if close
        # Compare stream output
        self.assertRastersNoDifference(self.stream_2, self.stream, 100)
        # Compare length_slope output
        self.assertRastersNoDifference(self.lengthslope_2, self.slopelength, 10)

    def test_watershedThreadholdfail(self):
        """Test if threshold of 0 or a negative is accepted

        The module should fail in this test, if it fails, test succeeds.
        """
        self.assertModuleFail(
            "r.watershed",
            elevation=self.elevation,
            threshold="0",
            stream=self.stream,
            overwrite=True,
            msg="Threshold value of 0 considered valid.",
        )
        self.assertModuleFail(
            "r.watershed",
            elevation=self.elevation,
            threshold="-1",
            stream=self.stream,
            overwrite=True,
            msg="Threshold value of -1 considered valid.",
        )

    def test_drainageDirection(self):
        """Test if the drainage direction is between -8 and 8."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="100000",
            drainage=self.drainage,
        )
        # Make sure the min/max is between -8 and 8
        self.assertRasterMinMax(
            self.drainage, -8, 8, msg="Direction must be between -8 and 8"
        )

    def test_accumulation_mfd(self):
        """Test MFD flow accumulation against reference statistics."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            accumulation=self.accumulation,
        )

        reference = {
            "n": 2602530,
            "null_cells": 16180,
            "min": -832420.692197234,
            "max": 429067.069562766,
            "mean": -343.747566806773,
            "stddev": 16259.307703876,
        }

        self.assertRasterFitsUnivar(
            self.accumulation,
            reference=reference,
            precision=0.001,
        )

    def test_accumulation_sfd(self):
        """Test SFD flow accumulation against reference statistics."""
        self.assertModule(
            "r.watershed",
            flags="s",
            elevation=self.elevation,
            threshold="10000",
            accumulation=self.accumulation,
        )

        reference = {
            "n": 2602530,
            "null_cells": 16180,
            "min": -832531,
            "max": 441312,
            "mean": -287.797973894633,
            "stddev": 16793.5124227608,
        }

        self.assertRasterFitsUnivar(
            self.accumulation,
            reference=reference,
            precision=0.001,
        )

    def test_basin_threshold_10k(self):
        """Test basin delineation with threshold=10000."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            basin=self.basin,
        )

        reference = {
            "n": 2456668,
            "null_cells": 162042,
            "min": 2,
            "max": 274,
            "mean": 142.826128723946,
            "stddev": 86.332946971229,
        }

        self.assertRasterFitsUnivar(
            self.basin,
            reference=reference,
            precision=0.001,
        )

    def test_basin_threshold_100k(self):
        """Test basin delineation with threshold=100000."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="100000",
            basin=self.basin,
        )

        reference = {
            "n": 2026515,
            "null_cells": 592195,
            "min": 2,
            "max": 20,
            "mean": 12.6790475274054,
            "stddev": 5.81111419543262,
        }

        self.assertRasterFitsUnivar(
            self.basin,
            reference=reference,
            precision=0.001,
        )

    def test_stream_network(self):
        """Test stream network delineation."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            stream=self.stream,
        )

        reference = {
            "n": 15740,
            "null_cells": 2602970,
            "min": 2,
            "max": 274,
            "mean": 141.941041931385,
            "stddev": 83.9583249945486,
        }

        self.assertRasterFitsUnivar(
            self.stream,
            reference=reference,
            precision=0.001,
        )

    def test_half_basin(self):
        """Test half basin delineation."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            half_basin=self.halfbasin,
        )

        reference = {
            "n": 2456668,
            "null_cells": 162042,
            "min": 1,
            "max": 274,
            "mean": 142.308489384809,
            "stddev": 86.3166284098813,
        }

        self.assertRasterFitsUnivar(
            self.halfbasin,
            reference=reference,
            precision=0.001,
        )

    def test_slope_steepness(self):
        """Test slope steepness calculation."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            slope_steepness=self.slopesteepness,
        )

        reference = {
            "n": 2602530,
            "null_cells": 16180,
            "min": 0.03,
            "max": 3.07919076172568,
            "mean": 0.153479116301884,
            "stddev": 0.163185729045392,
        }

        self.assertRasterFitsUnivar(
            self.slopesteepness,
            reference=reference,
            precision=0.001,
        )

    def test_length_slope(self):
        """Test Length Slope calculation."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            length_slope=self.slopelength,
        )

        reference = {
            "n": 2602530,
            "null_cells": 16180,
            "min": 0.03,
            "max": 5.98881244191164,
            "mean": 0.192025694191372,
            "stddev": 0.240997329983397,
        }

        self.assertRasterFitsUnivar(
            self.slopelength,
            reference=reference,
            precision=0.001,
        )

    def test_tci(self):
        """Test TCI calculation."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            tci=self.tci,
        )

        reference = {
            "n": 2596072,
            "null_cells": 22638,
            "min": 1.94904979310483,
            "max": 26.8104270376686,
            "mean": 6.97353811209655,
            "stddev": 2.28656463030412,
        }

        self.assertRasterFitsUnivar(
            self.tci,
            reference=reference,
            precision=0.001,
        )

    def test_spi(self):
        """Test SPI calculation."""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            spi=self.spi,
        )

        reference = {
            "n": 2596072,
            "null_cells": 22638,
            "min": 0.000144249450029743,
            "max": 1207802.52599239,
            "mean": 73.7040561277494,
            "stddev": 2750.87096268131,
        }

        self.assertRasterFitsUnivar(
            self.spi,
            reference=reference,
            precision=0.001,
        )

    def test_reproducibility(self):
        """Test that multiple runs produce identical results"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            basin=self.basin,
        )
        self.runModule("g.copy", raster=(self.basin, "basin_copy"))

        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            basin=self.basin,
            overwrite=True,
        )

        self.assertRastersNoDifference(
            self.basin, "basin_copy", precision=0, msg="Results are not reproducible"
        )
        self.runModule("g.remove", flags="f", type="raster", name="basin_copy")

    def test_minimum_threshold(self):
        """Test that minimum valid threshold (1) works"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="1",
            stream=self.stream,
        )
        self.assertRasterExists(self.stream, msg="Stream with threshold=1 not created")


if __name__ == "__main__":
    test()
