import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIEbSoilHeatFlux(TestCase):
    """Regression test suite for i.eb.soilheatflux."""

    albedo = "test_albedo"
    ndvi = "test_ndvi"
    temperature = "test_temperature"
    netradiation = "test_netradiation"
    localutctime = "test_localutctime"
    output = "test_soilheatflux"
    tmp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Initialize a temporary region and input rasters."""
        cls.use_temp_region()
        cls.runModule("g.region", e=10, w=0, s=0, n=10, rows=10, cols=10)

        cls._create_raster(cls.albedo, "0.1 + 0.3 * (row() + col()) / 20.0")
        cls._create_raster(cls.ndvi, "-0.2 + 0.8 * (row() + col()) / 20.0")
        cls._create_raster(cls.temperature, "300.0 + 10.0 * sin((row() + col()) * 0.1)")
        cls._create_raster(
            cls.netradiation, "400.0 + 20.0 * sin((row() + col()) * 0.1)"
        )
        cls._create_raster(cls.localutctime, "12.0 + 1.0 * cos((row() + col()) * 0.2)")

        cls.tmp_rasters.extend(
            [cls.albedo, cls.ndvi, cls.temperature, cls.netradiation, cls.localutctime]
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary raster maps and region after tests."""
        cls.tmp_rasters.append(cls.output)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.tmp_rasters)
        cls.del_temp_region()

    @classmethod
    def _create_raster(cls, name, expr):
        """Helper method to create a raster map."""
        cls.runModule("r.mapcalc", expression=f"{name} = float({expr})", overwrite=True)

    def _run_soilheatflux(self, ndvi=None, output=None, time=None, flags=""):
        """Run the i.eb.soilheatflux module with provided parameters."""
        return self.assertModule(
            "i.eb.soilheatflux",
            flags=flags,
            albedo=self.albedo,
            ndvi=ndvi or self.ndvi,
            temperature=self.temperature,
            netradiation=self.netradiation,
            localutctime=time or self.localutctime,
            output=output or self.output,
            overwrite=True,
        )

    def test_output_statistics(self):
        """Verify statistics of the output map against reference values."""
        self._run_soilheatflux()

        reference = {
            "min": 48.935794,
            "max": 64.674187,
            "mean": 59.542719,
            "stddev": 4.095612,
        }
        self.assertRasterFitsUnivar(self.output, reference, precision=1e-6)

    def test_heatflux_decreases_with_high_ndvi(self):
        """Validate that soil heat flux is lower for higher NDVI (dense vegetation)."""
        self._create_raster("ndvi_low", "0.2")
        self.tmp_rasters.append("ndvi_low")

        self._run_soilheatflux(ndvi="ndvi_low", output="g0_low")
        self.tmp_rasters.append("g0_low")
        g0_low = gs.parse_command("r.univar", map="g0_low", format="json")

        self._create_raster("ndvi_high", "0.8")
        self.tmp_rasters.append("ndvi_high")

        self._run_soilheatflux(ndvi="ndvi_high", output="g0_high")
        self.tmp_rasters.append("g0_high")
        g0_high = gs.parse_command("r.univar", map="g0_high", format="json")

        self.assertGreater(
            g0_low["mean"],
            g0_high["mean"],
            "Expected lower soil heat flux for higher NDVI.",
        )

    def test_heatflux_increases_with_time(self):
        """Check that soil heat flux increases with local overpass time."""
        self._create_raster("time_early", "8.0")
        self.tmp_rasters.append("time_early")

        self._run_soilheatflux(time="time_early", output="g0_early")
        self.tmp_rasters.append("g0_early")
        g0_early = gs.parse_command("r.univar", map="g0_early", format="json")

        self._create_raster("time_noon", "12.0")
        self.tmp_rasters.append("time_noon")

        self._run_soilheatflux(time="time_noon", output="g0_noon")
        self.tmp_rasters.append("g0_noon")
        g0_noon = gs.parse_command("r.univar", map="g0_noon", format="json")

        self.assertGreater(
            g0_noon["mean"],
            g0_early["mean"],
            "Soil heat flux should be higher at noon.",
        )

    def test_roerink_correction_flag(self):
        """Test that the -r flag applies the expected correction."""
        self._run_soilheatflux(output="g0_base")
        self.tmp_rasters.append("g0_base")

        self._run_soilheatflux(output="g0_roerink", flags="r")
        self.tmp_rasters.append("g0_roerink")

        self.runModule(
            "r.mapcalc",
            expression="g0_expected = 1.43 * g0_base - 0.0845",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="g0_diff = abs(g0_expected - g0_roerink)",
            overwrite=True,
        )
        self.tmp_rasters.extend(["g0_expected", "g0_diff"])

        stats = gs.parse_command("r.univar", map="g0_diff", format="json")
        self.assertLess(
            stats["max"],
            1e-6,
            "Roerink correction did not match expected transformation.",
        )


if __name__ == "__main__":
    test()
