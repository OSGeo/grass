from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIEmissivity(TestCase):
    """Test case for the i.emissivity module."""

    input_raster = "ndvi_test_map"
    output_raster = "emissivity_test_output"
    reference_raster = "emissivity_reference_map"
    temp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up an input raster and configure test environment."""
        cls.use_temp_region()

        cls.runModule(
            "g.region",
            n=10,
            s=0,
            e=10,
            w=0,
            res=1,
            flags="p",
        )

        cls.runModule(
            "r.mapcalc",
            expression=(f"{cls.input_raster} = if(row() <= 5, 0.2, 0.7)"),
            overwrite=True,
        )
        cls.temp_rasters.append(cls.input_raster)

        cls.runModule(
            "r.mapcalc",
            expression=(f"{cls.reference_raster} = if(row() <= 5, 0.9, 0.95)"),
            overwrite=True,
        )
        cls.temp_rasters.append(cls.reference_raster)

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data and reset the region."""
        cls.del_temp_region()
        for raster in cls.temp_rasters + [cls.output_raster]:
            cls.runModule("g.remove", type="raster", name=raster, flags="f")

    def test_emissivity_ndvi_range(self):
        """Test with NDVI values in the valid range."""
        self.runModule(
            "r.mapcalc",
            expression=(
                f"{self.input_raster} = if(col() == 1, 0.16, "
                f"if(col() == 2, 0.74, 0.45))"
            ),
            overwrite=True,
        )

        self.assertModule(
            "i.emissivity",
            input=self.input_raster,
            output=self.output_raster,
            overwrite=True,
        )

        self.runModule(
            "r.mapcalc",
            expression=(
                f"{self.reference_raster} = if({self.input_raster} < 0.16, 0.97, "
                f"if({self.input_raster} > 0.74, 0.99, "
                f"0.97 + 0.003 * {self.input_raster}))"
            ),
            overwrite=True,
        )

        self.runModule(
            "r.mapcalc",
            expression=(
                f"diff_raster = abs({self.output_raster} - {self.reference_raster})"
            ),
            overwrite=True,
        )
        self.assertModule("r.univar", map="diff_raster", flags="e")
        self.assertRasterMinMax("diff_raster", 0, 0.05)

    def test_complex_mask(self):
        """Test with a complex mask involving irregular shape and regions."""
        self.runModule(
            "r.mapcalc",
            expression=("MASK = if((row() + col()) % 3 == 0 || row() < 3, 1, null())"),
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
            expression=(
                f"masked_reference = if(isnull(MASK), null(), {self.reference_raster})"
            ),
            overwrite=True,
        )
        self.temp_rasters.append("masked_reference")

        self.runModule(
            "r.mapcalc",
            expression=(f"diff_raster = abs({self.output_raster} - masked_reference)"),
            overwrite=True,
        )
        self.temp_rasters.append("diff_raster")

        self.assertModule("r.univar", map="diff_raster", flags="e")
        self.assertRasterMinMax("diff_raster", 0, 0.05)

        self.runModule("r.mask", flags="r")

    def test_region_resolution(self):
        """Test the module behaviour with different region resolutions."""
        for res in [1, 0.1]:
            with self.subTest(res=res):
                self.runModule(
                    "g.region",
                    n=10,
                    s=0,
                    e=10,
                    w=0,
                    res=res,
                )
                self.runModule(
                    "r.mapcalc",
                    expression=f"{self.input_raster} = 0.6",
                    overwrite=True,
                )
                self.assertModule(
                    "i.emissivity",
                    input=self.input_raster,
                    output=self.output_raster,
                    overwrite=True,
                )
                self.assertRasterMinMax(self.output_raster, 0, 1)

    def test_large_raster_performance(self):
        """Assess performance with a larger raster."""
        self.runModule("g.region", n=90, s=-90, e=180, w=-180, rows=1000, cols=1000)
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_raster} = col()",
            overwrite=True,
        )

        self.assertModule(
            "i.emissivity",
            input=self.input_raster,
            output=self.output_raster,
            overwrite=True,
        )

        self.assertRasterExists(self.output_raster)

    def test_extreme_ndvi_values(self):
        """Test with extreme NDVI values beyond the valid range."""
        self.runModule(
            "r.mapcalc",
            expression=(
                f"{self.input_raster} = if(col() == 1, -1, "
                f"if(col() == 2, 1.2, 0.5))"
            ),
            overwrite=True,
        )
        self.assertModule(
            "i.emissivity",
            input=self.input_raster,
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterMinMax(self.output_raster, 0, 1)


if __name__ == "__main__":
    test()
