import math

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestR3CrossRast(TestCase):
    """Regression tests for the r3.cross.rast GRASS module."""

    tmp_3d_rasters = []
    tmp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up the test environment and create input raster maps."""
        cls.use_temp_region()
        cls.runModule(
            "g.region", s=0, n=10, w=0, e=10, b=0, t=10, res=1, res3=1, flags="p"
        )

        cls.runModule("r3.mapcalc", expression="voxel = z()", overwrite=True)
        cls.runModule("r.mapcalc", expression="elev_flat = 5", overwrite=True)
        cls.runModule(
            "r.mapcalc", expression="elev_diag = (row() + col()) / 2", overwrite=True
        )
        cls.runModule("r.mapcalc", expression="elev_high = 20", overwrite=True)
        cls.runModule(
            "r.mapcalc",
            expression="elev_nulls = if(col() < 5, 5, null())",
            overwrite=True,
        )

        cls.tmp_3d_rasters.extend(["voxel"])
        cls.tmp_rasters.extend(["elev_flat", "elev_diag", "elev_high", "elev_nulls"])

    @classmethod
    def tearDownClass(cls):
        """Clean up the test environment by removing created maps."""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.tmp_rasters)
        cls.runModule("g.remove", flags="f", type="raster_3d", name=cls.tmp_3d_rasters)
        cls.del_temp_region()

    def test_cross_section_at_flat_plane(self):
        """Verify cross-section extraction at constant Z elevation."""
        self.assertModule(
            "r3.cross.rast", input="voxel", elevation="elev_flat", output="out_flat"
        )

        self.assertRasterExists("out_flat")
        self.tmp_rasters.append("out_flat")
        expected_stats = {"min": 5.5, "max": 5.5, "mean": 5.5, "stddev": 0.0}
        self.assertRasterFitsUnivar("out_flat", expected_stats, precision=1e-6)

    def test_cross_section_with_diagonal_elevation(self):
        """Check correctness of cross-section using a sloped diagonal elevation map."""
        self.assertModule(
            "r3.cross.rast", input="voxel", elevation="elev_diag", output="out_diag"
        )
        self.assertRasterExists("out_diag")
        self.tmp_rasters.append("out_diag")
        expected_stats = {"min": 1.5, "max": 9.5, "mean": 5.702020, "stddev": 1.999897}

        self.assertRasterFitsUnivar("out_diag", expected_stats, precision=1e-6)

    def test_elevation_above_voxel_returns_nulls(self):
        """Ensure elevation values above the 3D extent produce all NULLs in output."""
        self.assertModule(
            "r3.cross.rast", input="voxel", elevation="elev_high", output="out_high"
        )
        self.assertRasterExists("out_high")
        self.tmp_rasters.append("out_high")
        stats = gs.parse_command("r.univar", map="out_high", flags="g")
        self.assertTrue(math.isnan(float(stats["min"])))
        self.assertEqual(stats["cells"], stats["null_cells"])

    def test_null_cells_in_elevation_propagate(self):
        """Check that NULL values in the elevation map yield NULLs in the output."""
        self.assertModule(
            "r3.cross.rast", input="voxel", elevation="elev_nulls", output="out_nulls"
        )
        self.assertRasterExists("out_nulls")
        self.tmp_rasters.append("out_nulls")
        stats = gs.parse_command("r.univar", map="out_nulls", format="json")
        self.assertGreater(stats["null_cells"], 0)
        self.assertGreater(stats["cells"], stats["null_cells"])


if __name__ == "__main__":
    test()
