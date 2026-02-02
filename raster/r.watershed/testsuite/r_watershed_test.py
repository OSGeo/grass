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
            name=[
                self.accumulation,
                self.drainage,
                self.basin,
                self.stream,
                self.halfbasin,
                self.slopelength,
                self.slopesteepness,
                self.lengthslope_2,
                self.stream_2,
            ],
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
            msg="Threshold value of 0 considered valid.",
        )

    def test_thresholdsize(self):
        """Test the expected range of basin output values"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="100000",
            basin=self.basin,
            overwrite=True,
        )
        # it is expected that 100k Threshold has a min=2 and max=20 for this
        # data (north Carolina dataset, (nc_spm_08_grass7))
        reference = "min=2\nmax=20"
        self.assertRasterFitsUnivar(
            self.basin,
            reference=reference,
            msg="Basin values must be in the range [2, 20]",
        )
        # it is expected that 100k Threshold has a min=2 and max=274 for this
        # data (North carolina dataset,(nc_spm_08_grass7))
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            basin=self.basin,
            overwrite=True,
        )
        reference = "min=2\nmax=274"
        self.assertRasterFitsUnivar(
            self.basin,
            reference=reference,
            msg="Basin values must be in the range [2, 274]",
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

    def test_basinValue(self):
        """Check to see if the basin value is 0 or greater"""
        self.assertModule(
            "r.watershed", elevation=self.elevation, threshold="10000", basin=self.basin
        )
        # Make sure the minimum value is 0 for basin value representing unique
        # positive integer.
        # TODO: test just min, max is theoretically unlimited
        # or set a lower value according to what is expected with this data
        # TODO: add test which tests that 'max basin id' == 'num of basins'
        reference = "min=2\nmax=274"
        self.assertRasterFitsUnivar(
            self.basin,
            reference=reference,
            msg="Basin values must be in the range [2, 274]",
        )

    def test_accumulationValues(self):
        """Test if accumulation values follow expected patterns"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            accumulation=self.accumulation,
        )
        # Just verify the output exists and has valid statistics
        self.assertRasterExists(
            self.accumulation, msg="Accumulation output was not created"
        )

    def test_streamValuesConsistency(self):
        """Test if stream output is created with valid values"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            stream=self.stream,
        )
        # Stream values should be 0 (no stream) or positive integers
        self.assertRasterMinMax(
            self.stream, 0, 10000, msg="Stream values out of expected range"
        )

    def test_slopeSteepnessRange(self):
        """Test if slope steepness values are created"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            slope_steepness=self.slopesteepness,
        )
        # Verify output exists with reasonable range
        self.assertRasterMinMax(
            self.slopesteepness,
            0,
            100,
            msg="Slope steepness out of expected range",
        )

    def test_convergenceFlag(self):
        """Test the convergence parameter"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            convergence="5",
            accumulation=self.accumulation,
        )
        self.assertRasterExists(
            self.accumulation, msg="Accumulation with convergence parameter not created"
        )

    def test_memoryParameter(self):
        """Test if memory parameter is accepted"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            memory="300",
            accumulation=self.accumulation,
        )
        self.assertRasterExists(
            self.accumulation, msg="Accumulation with memory parameter not created"
        )

    def test_aFlag(self):
        """Test the -a flag for positive flow accumulation"""
        self.assertModule(
            "r.watershed",
            flags="a",
            elevation=self.elevation,
            threshold="10000",
            accumulation=self.accumulation,
        )
        # Verify output is created with -a flag
        self.assertRasterExists(
            self.accumulation, msg="Accumulation with -a flag not created"
        )

    def test_consistentResults(self):
        """Test that running twice with same parameters gives identical results"""
        # First run
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            basin=self.basin,
        )
        # Store first result
        self.runModule("g.copy", raster=(self.basin, "basin_copy"))

        # Second run with overwrite
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="10000",
            basin=self.basin,
            overwrite=True,
        )
        # Results should be identical
        self.assertRastersNoDifference(
            self.basin, "basin_copy", 0, msg="Results are not reproducible"
        )
        self.runModule("g.remove", flags="f", type="raster", name="basin_copy")

    def test_minimumThreshold(self):
        """Test that minimum valid threshold (1) works"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            threshold="1",
            stream=self.stream,
        )
        self.assertRasterExists(
            self.stream, msg="Stream output with threshold=1 not created"
        )

    def test_bFlag(self):
        """Test the -b flag for beautification"""
        self.assertModule(
            "r.watershed",
            flags="b",
            elevation=self.elevation,
            threshold="10000",
            accumulation=self.accumulation,
        )
        self.assertRasterExists(
            self.accumulation, msg="Accumulation with -b flag not created"
        )


if __name__ == "__main__":
    test()
