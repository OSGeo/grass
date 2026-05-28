import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestImageMosaic(TestCase):
    """Regression testsuite for i.image.mosaic GRASS module."""

    tmp_rasters = []
    output = "mosaic_output"

    @classmethod
    def setUpClass(cls):
        """Set up input rasters and configure test environment."""
        gs.run_command(
            "r.mapcalc", expression="raster1 = if(row() <=5, 1, null())", overwrite=True
        )
        gs.run_command(
            "r.mapcalc", expression="raster2 = if(row() <=5,11,null())", overwrite=True
        )
        gs.run_command(
            "r.mapcalc", expression="raster3 = if(row() <=5,null(),30)", overwrite=True
        )
        cls.tmp_rasters = ["raster1", "raster2", "raster3"]
        gs.run_command("g.region", raster="raster1,raster2,raster3")

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data."""
        cls.tmp_rasters.append(cls.output)
        gs.run_command("g.remove", flags="f", type="raster", name=cls.tmp_rasters)

    def test_mosaic_two_inputs_with_partial_nulls(self):
        """
        Mosaicking raster1 and raster2 where both have NULLs in lower rows.
        Verifies that output contains only raster1 values (no fallback).
        """
        self.runModule(
            "i.image.mosaic",
            input="raster1,raster2",
            output=f"{self.output}_two_inputs",
            overwrite=True,
        )
        self.tmp_rasters.append(f"{self.output}_two_inputs")

        stats = gs.parse_command("r.univar", map=f"{self.output}_two_inputs", flags="g")
        self.assertEqual(float(stats["min"]), 1.0)
        self.assertEqual(float(stats["max"]), 1.0)

    def test_mosaic_three_inputs_with_raster3_fallback(self):
        """
        Mosaicking all three rasters.
        Verifies that raster3 fills cells where raster1 and raster2 are NULL.
        """
        self.runModule(
            "i.image.mosaic",
            input="raster1,raster2,raster3",
            output=f"{self.output}_three_inputs",
            overwrite=True,
        )

        self.tmp_rasters.append(f"{self.output}_three_inputs")
        stats = gs.parse_command(
            "r.univar", map=f"{self.output}_three_inputs", flags="g"
        )
        self.assertEqual(float(stats["min"]), 1.0)
        self.assertEqual(float(stats["max"]), 44.0)

    def test_mosaic_single_input_identity(self):
        """
        Mosaicking a single raster should reproduce the raster unchanged.
        """
        self.runModule(
            "i.image.mosaic",
            input="raster2",
            output=f"{self.output}_single_input",
            overwrite=True,
        )

        self.tmp_rasters.append(f"{self.output}_single_input")
        stats_orig = gs.parse_command("r.univar", map="raster2", flags="g")
        stats_out = gs.parse_command(
            "r.univar", map=f"{self.output}_single_input", flags="g"
        )
        self.assertEqual(float(stats_orig["min"]), float(stats_out["min"]))
        self.assertEqual(float(stats_orig["max"]), float(stats_out["max"]))

    def test_mosaic_color_table_generated(self):
        """
        Verifies that the output raster has the expected color table.
        """
        self.runModule(
            "i.image.mosaic",
            input="raster1,raster2,raster3",
            output=self.output,
            overwrite=True,
        )

        self.tmp_rasters.append(self.output)

        color_rules = gs.read_command("r.colors.out", map=self.output).splitlines()
        expected_rules = [
            "1 252:231:36",
            "13 252:231:36",
            "44 252:231:36",
            "nv 255:255:255",
            "default 255:255:255",
        ]

        self.assertEqual(
            color_rules,
            expected_rules,
            msg="Color table does not match expected rules.\n"
            f"Actual:\n{color_rules}\nExpected:\n{expected_rules}",
        )


if __name__ == "__main__":
    test()
