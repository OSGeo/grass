import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import math


class TestASTERToarRegression(TestCase):
    """Regression test suite for i.aster.toar GRASS module."""

    input_bands = [
        f"b{i}" for i in [1, 2, "3N", "3B", 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]
    ]
    output_base = "toar_output"
    dayofyear = 150
    sun_elevation = 45.0
    seed = 42

    expected = {
        "radiance": {
            10: {"min": -0.006343, "max": 1.732233, "sum": 81.572277},
            11: {"min": -0.001997, "max": 1.716536, "sum": 92.402068},
            12: {"min": 0.007842, "max": 1.641329, "sum": 78.981212},
            13: {"min": -0.004269, "max": 1.407337, "sum": 67.583852},
            14: {"min": 0.024680, "max": 1.310518, "sum": 72.093405},
        },
        "vnir_high": {
            1: {"min": 0.002843, "max": 0.423546, "sum": 20.819018},
            2: {"min": 0.016028, "max": 0.523680, "sum": 24.802390},
            "3N": {"min": -0.001800, "max": 0.463181, "sum": 21.425668},
            "3B": {"min": 0.001090, "max": 0.462429, "sum": 24.986496},
            4: {"min": -0.001790, "max": 0.553876, "sum": 28.228600},
            5: {"min": 0.008488, "max": 0.462331, "sum": 25.353367},
            6: {"min": 0.002802, "max": 0.438120, "sum": 24.748814},
            7: {"min": 0.000297, "max": 0.463403, "sum": 23.870616},
            8: {"min": 0.001802, "max": 0.361291, "sum": 17.144211},
            9: {"min": 0.003090, "max": 0.305789, "sum": 14.029806},
        },
        "vnir_low": {
            1: {"min": 0.009462, "max": 1.409734, "sum": 69.294068},
            2: {"min": 0.042789, "max": 1.397959, "sum": 66.209771},
            "3N": {"min": -0.004894, "max": 1.259240, "sum": 58.249453},
            "3B": {"min": 0.002965, "max": 1.257195, "sum": 67.930192},
            4: {"min": -0.004777, "max": 1.477684, "sum": 75.310894},
            5: {"min": 0.099762, "max": 5.433718, "sum": 297.974926},
            6: {"min": 0.034922, "max": 5.459007, "sum": 308.371814},
            7: {"min": 0.003300, "max": 5.145480, "sum": 265.051662},
            8: {"min": 0.021130, "max": 4.235232, "sum": 200.972820},
            9: {"min": 0.051514, "max": 5.096499, "sum": 233.830104},
        },
    }

    @classmethod
    def setUpClass(cls):
        """Create input raster maps and set up the region."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

        for idx, band in enumerate(cls.input_bands):
            cls.runModule(
                "r.surf.random",
                output=band,
                max=255,
                seed=cls.seed + idx,
                overwrite=True,
            )

    @classmethod
    def tearDownClass(cls):
        """Remove all temporary rasters created by the tests and restore region."""
        cls.runModule(
            "g.remove", type="raster", pattern=f"{cls.output_base}.*", flags="f"
        )
        cls.runModule(
            "g.remove", type="raster", name=",".join(cls.input_bands), flags="f"
        )
        cls.runModule(
            "g.remove", type="raster", pattern=f"{cls.output_base}_45.*", flags="f"
        )
        cls.runModule(
            "g.remove", type="raster", pattern=f"{cls.output_base}_60.*", flags="f"
        )
        cls.del_temp_region()

    def _assert_band_stats(self, test_case, bands):
        """Helper function to assert that raster statistics match expected regression values."""
        for band in bands:
            stats = self.expected[test_case][band]
            self.assertRasterExists(f"{self.output_base}.{band}")
            self.assertRasterFitsUnivar(
                f"{self.output_base}.{band}", reference=stats, precision=1e-6
            )

    def test_radiance_output(self):
        """Test output in radiance mode (-r flag) for thermal bands (10-14)."""
        self.assertModule(
            "i.aster.toar",
            input=",".join(self.input_bands),
            output=self.output_base,
            dayofyear=self.dayofyear,
            sun_elevation=self.sun_elevation,
            flags="r",
        )
        self._assert_band_stats("radiance", [10, 11, 12, 13, 14])

        for band in [10, 11, 12, 13, 14]:
            info = gs.raster_info(f"{self.output_base}.{band}")
            self.assertIn(info["datatype"], ["FCELL", "DCELL"])
            self.assertGreater(
                info["max"], 1.0, "Radiance max too low for thermal bands"
            )
            self.assertLess(info["max"], 5.0, "Radiance max too high for thermal bands")

    def test_high_gain_modes(self):
        """Test VNIR and SWIR high gain modes (-a -b).
        Ensure that reflectance outputs match regression expectations."""
        self.assertModule(
            "i.aster.toar",
            input=",".join(self.input_bands),
            output=self.output_base,
            dayofyear=self.dayofyear,
            sun_elevation=self.sun_elevation,
            flags="ab",
        )
        self._assert_band_stats("vnir_high", [1, 2, "3N", "3B", 4, 5, 6, 7, 8, 9])

    def test_low_gain_modes(self):
        """Test VNIR and SWIR low gain modes (-c -d -e).
        Verify output matches regression expectations under low gain."""
        self.assertModule(
            "i.aster.toar",
            input=",".join(self.input_bands),
            output=self.output_base,
            dayofyear=self.dayofyear,
            sun_elevation=self.sun_elevation,
            flags="cde",
        )
        self._assert_band_stats("vnir_low", [1, 2, "3N", "3B", 4, 5, 6, 7, 8, 9])

    def test_sun_elevation_effect_on_reflectance(self):
        """Test the theoretical effect of sun_elevation on reflectance.

        For reflectance outputs (using high gain flags "ab"),:
        reflectance ∝ 1/cos(90° - sun_elevation),
        Thus, for sun_elevation of 45° and 60°, the ratio of
        outputs should be approximately:
        reflectance60/reflectance45 = cos(45°)/cos(30°) ≈ 0.8165.
        """

        output_45 = self.output_base + "_45"
        output_60 = self.output_base + "_60"

        self.assertModule(
            "i.aster.toar",
            input=",".join(self.input_bands),
            output=output_45,
            dayofyear=self.dayofyear,
            sun_elevation=45.0,
            flags="ab",
        )

        self.assertModule(
            "i.aster.toar",
            input=",".join(self.input_bands),
            output=output_60,
            dayofyear=self.dayofyear,
            sun_elevation=60.0,
            flags="ab",
        )

        stats_45 = gs.parse_command("r.univar", map=f"{output_45}.1", format="json")
        stats_60 = gs.parse_command("r.univar", map=f"{output_60}.1", format="json")

        ratio = stats_60["sum"] / stats_45["sum"]
        expected_ratio = math.cos(math.radians(45)) / math.cos(math.radians(30))

        self.assertAlmostEqual(
            ratio,
            expected_ratio,
            places=5,
            msg=f"Expected ratio {expected_ratio:.5f}, got {ratio:.5f}",
        )


if __name__ == "__main__":
    test()
