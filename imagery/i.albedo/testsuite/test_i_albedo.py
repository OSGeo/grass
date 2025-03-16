from grass.script import core
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from parameterized import parameterized


class TestIAlbedo(TestCase):
    """Regression tests for the i.albedo GRASS GIS module."""

    output_raster = "albedo_output"

    def setUp(self):
        """Initialize temporary region with 10x10 grid."""
        self.use_temp_region()
        self.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

    def tearDown(self):
        """Remove temporary maps and restore original region."""
        self.runModule("g.remove", type="raster", flags="f", pattern="test_*")
        self.runModule("g.remove", type="raster", flags="f", name=self.output_raster)
        self.del_temp_region()

    def _create_input_rasters(self, sensor, values=None):
        """Generate test rasters for sensor bands."""
        bands = {
            "noaa": ["test_red", "test_nir"],
            "modis": [f"test_b{i}" for i in range(1, 8)],
            "landsat": [f"test_b{i}" for i in [1, 2, 3, 4, 5, 7]],
            "aster": [f"test_b{i}" for i in [1, 3, 5, 6, 8, 9]],
        }.get(sensor)
        values = values or [0.1 * (i + 1) for i in range(len(bands))]
        for band, value in zip(bands, values):
            self.runModule("r.mapcalc", expression=f"{band} = {value}", overwrite=True)
        return bands

    @parameterized.expand(
        [
            (
                "modis",
                "m",
                [0.1 * i for i in range(1, 8)],
                {"mean": 0.38995, "sum": 38.995},
            ),
            (
                "landsat",
                "l",
                [0.1 * i for i in [1, 2, 3, 4, 5, 7]],
                {"mean": 0.2406, "sum": 24.06},
            ),
            ("noaa", "n", [0.5, 0.8], {"mean": 0.311, "sum": 31.0999989}),
            (
                "aster",
                "a",
                [0.1 * i for i in [1, 3, 5, 6, 8, 9]],
                {"mean": 0.2297, "sum": 22.9699999},
            ),
        ]
    )
    def test_sensor_albedo(self, sensor, flag, values, reference_stats):
        """Verify albedo calculations for MODIS, Landsat, NOAA, and ASTER sensors against expected statistical results."""
        input_rasters = self._create_input_rasters(sensor, values)
        self.assertModule(
            "i.albedo",
            input=",".join(input_rasters),
            output=self.output_raster,
            flags=flag,
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            raster=self.output_raster,
            reference=reference_stats,
            precision=1e-6,
        )

    def test_albedo_output_range(self):
        """Ensure computed albedo values remain within valid 0-1 range."""
        input_rasters = self._create_input_rasters("landsat")
        self.assertModule(
            "i.albedo",
            input=",".join(input_rasters),
            output=self.output_raster,
            flags="l",
            overwrite=True,
        )
        stats = core.parse_command("r.info", map=self.output_raster, flags="r")
        min_val = float(stats["min"])
        max_val = float(stats["max"])
        self.assertGreaterEqual(min_val, 0, "Albedo values < 0 detected")
        self.assertLessEqual(max_val, 1, "Albedo values > 1 detected")

    def test_landsat_aggressive_mode_comparison(self):
        """Compare standard vs aggressive processing modes for dynamic range reduction and mean value differences."""
        bands = [f"test_b{i}" for i in [1, 2, 3, 4, 5, 7]]
        for idx, band in enumerate(bands):
            self.runModule(
                "r.mapcalc",
                expression=f"{band}=0.1*({idx + 1})+rand(0,0.15)",
                seed=42,
                overwrite=True,
            )
        for flag, out in [("l", "albedo_standard"), ("lc", "albedo_aggressive")]:
            self.assertModule(
                "i.albedo",
                input=",".join(bands),
                output=out,
                flags=flag,
                overwrite=True,
            )

        std_stats = core.parse_command("r.univar", map="albedo_standard", flags="g")
        agg_stats = core.parse_command("r.univar", map="albedo_aggressive", flags="g")
        self.assertLess(
            float(agg_stats["max"]) - float(agg_stats["min"]),
            float(std_stats["max"]) - float(std_stats["min"]),
            "Aggressive mode should reduce dynamic range",
        )
        self.assertNotAlmostEqual(
            float(std_stats["mean"]),
            float(agg_stats["mean"]),
            places=3,
            msg="Aggressive mode should shift the mean",
        )

    def test_linearity(self):
        """Validate linear reflectance scaling by comparing that doubled input results to doubled output expectations."""

        def run_test(values, out_name):
            bands = self._create_input_rasters("landsat", values)
            self.assertModule(
                "i.albedo",
                input=",".join(bands),
                output=out_name,
                flags="l",
                overwrite=True,
            )

        run_test([0.1] * 6, "albedo_low")
        run_test([0.2] * 6, "albedo_high")
        self.runModule(
            "r.mapcalc", expression="diff=albedo_high-2*albedo_low", overwrite=True
        )
        diff_stats = core.parse_command("r.univar", map="diff", flags="g")
        max_diff = float(diff_stats["max"])
        min_diff = float(diff_stats["min"])
        self.assertAlmostEqual(
            max_diff,
            0.0,
            places=2,
            msg="Albedo does not scale linearly with input (max difference)",
        )
        self.assertAlmostEqual(
            min_diff,
            0.0,
            places=2,
            msg="Albedo does not scale linearly with input (min difference)",
        )


if __name__ == "__main__":
    test()
