import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRRandomSurface(TestCase):
    """This test uses mask, so it is in a separate file from the rest

    While mask manager helps, the separation cannot be perfect due not accepting env
    in grass.gunittest, so grass.gunittest per-mapset separation for files is required.
    """

    output_raster = "random_surface_test"
    mask_raster = "random_surface_mask"

    @classmethod
    def setUpClass(cls):
        """Setup input rasters and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=20, s=0, e=20, w=0, rows=20, cols=20)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.mask_raster} = if(row() < 10, 1, null())",
            quiet=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove generated rasters and reset test environment."""
        cls.runModule(
            "g.remove",
            type="raster",
            name=[cls.mask_raster],
            flags="f",
            quiet=True,
        )
        cls.del_temp_region()

    def tearDown(self):
        """Remove the created rasters after each test."""
        self.runModule(
            "g.remove",
            type="raster",
            name=self.output_raster,
            flags="f",
            quiet=True,
        )

    def test_with_mask(self):
        """Test with a raster mask applied and validate stats."""
        with gs.MaskManager():
            self.runModule("r.mask", raster=self.mask_raster)
            self.assertModule(
                "r.random.surface", output=self.output_raster, seed=42, overwrite=True
            )

        reference_stats = {
            "mean": 135.695,
            "min": 1,
            "max": 255,
            "cells": 400,
        }

        self.assertRasterFitsUnivar(
            raster=self.output_raster, reference=reference_stats, precision=1e-6
        )


if __name__ == "__main__":
    test()
