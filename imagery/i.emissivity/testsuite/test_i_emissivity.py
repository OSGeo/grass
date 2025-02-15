from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIEmissivity(TestCase):
    """Test case for the i.emissivity module."""

    @classmethod
    def setUpClass(cls):
        """Set up input rasters and configure test environment."""
        cls.use_temp_region()

        cls.input_raster = "ndvi_test_map"
        cls.output_raster = "emissivity_test_output"
        cls.reference_raster = "emissivity_reference_map"
        cls.temp_rasters = []

        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

        # Predefined NDVI test cases
        cls.ndvi_cases = {
            "default": "if(row() <= 5, 0.2, 0.7)",
            "valid_range": "if(col() == 1, 0.16, if(col() == 2, 0.74, 0.45))",
            "extreme_values": "if(col() == 1, -1, if(col() == 2, 1.2, 0.5))",
        }

        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_raster} = {cls.ndvi_cases['default']}",
            overwrite=True,
        )
        cls.temp_rasters.append(cls.input_raster)

        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.reference_raster} = if(row() <= 5, 0.9, 0.95)",
            overwrite=True,
        )
        cls.temp_rasters.append(cls.reference_raster)

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data and reset the region."""
        for raster in cls.temp_rasters + [cls.output_raster]:
            cls.runModule("g.remove", type="raster", name=raster, flags="f")
        cls.del_temp_region()

    def test_emissivity_ndvi_range(self):
        """Test with NDVI values in the valid range using reference statistics."""
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_raster} = {self.ndvi_cases['valid_range']}",
            overwrite=True,
        )
        self.assertModule(
            "i.emissivity",
            input=self.input_raster,
            output=self.output_raster,
            overwrite=True,
        )

        # Reference statistics for the expected output
        # These values are derived from the emissivity formula:
        # For col 1 (NDVI = 0.16): emissivity = 0.97
        # For col 2 (NDVI = 0.74): emissivity = 0.99
        # For col 3+ (NDVI = 0.45): emissivity = 0.97 + 0.003 * 0.45 = 0.97135
        reference_stats = {
            "mean": 0.97712,  # (0.97 + 0.99 + 0.97135) / 3
            "min": 0.92,
            "max": 0.99,
        }
        self.assertRasterFitsUnivar(
            raster=self.output_raster, reference=reference_stats, precision=1e-2
        )

    def test_complex_mask(self):
        """Test with a complex mask involving irregular shape and regions."""
        self.runModule(
            "r.mapcalc",
            expression="MASK = if((row() + col()) % 3 == 0 || row() < 3, 1, null())",
            overwrite=True,
        )
        self.temp_rasters.append("MASK")

        self.assertModule(
            "i.emissivity",
            input=self.input_raster,
            output=self.output_raster,
            overwrite=True,
        )

        self.runModule(
            "r.mapcalc",
            expression=f"masked_reference = if(isnull(MASK), null(), {self.reference_raster})",
            overwrite=True,
        )
        self.temp_rasters.append("masked_reference")

        self.runModule(
            "r.mapcalc",
            expression=f"diff_raster = abs({self.output_raster} - masked_reference)",
            overwrite=True,
        )
        self.temp_rasters.append("diff_raster")

        self.assertRasterMinMax("diff_raster", 0, 0.05)
        self.runModule("r.mask", flags="r")

    def test_region_resolution(self):
        """Test the module behaviour with different region resolutions."""
        results = {}
        for res in [1, 0.1]:
            with self.subTest(res=res):
                self.runModule("g.region", n=10, s=0, e=10, w=0, res=res)
                self.runModule(
                    "r.mapcalc", expression=f"{self.input_raster} = 0.6", overwrite=True
                )
                self.assertModule(
                    "i.emissivity",
                    input=self.input_raster,
                    output=self.output_raster,
                    overwrite=True,
                )
                results[res] = self.assertRasterMinMax(self.output_raster, 0, 1)

        self.assertAlmostEqual(results[1], results[0.1], delta=0.01)

    def test_partial_null_values(self):
        """Test the module behavior when NDVI has null values."""
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_raster} = if(col() % 2 == 0, null(), 0.5)",
            overwrite=True,
        )
        self.assertModule(
            "i.emissivity",
            input=self.input_raster,
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)
        self.assertRasterMinMax(self.output_raster, 0, 1)

    def test_extreme_ndvi_values(self):
        """Test with extreme NDVI values beyond the valid range."""
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_raster} = {self.ndvi_cases['extreme_values']}",
            overwrite=True,
        )
        self.assertModule(
            "i.emissivity",
            input=self.input_raster,
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)
        self.assertRasterMinMax(self.output_raster, 0, 1)


if __name__ == "__main__":
    test()
