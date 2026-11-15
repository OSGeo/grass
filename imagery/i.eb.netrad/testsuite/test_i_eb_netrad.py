import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIEbNetrad(TestCase):
    """Regression test suite for i.eb.netrad module."""

    albedo = "test_albedo"
    ndvi = "test_ndvi"
    temperature = "test_temperature"
    localutctime = "test_localutctime"
    temperaturedifference2m = "test_temperaturediff2m"
    emissivity = "test_emissivity"
    transmissivity = "test_transmissivity"
    dayofyear = "test_dayofyear"
    sunzenithangle = "test_sunzenithangle"
    output = "test_netrad_output"
    tmp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up the test environment and generate input raster maps for net radiation calculation."""
        cls.use_temp_region()
        cls.runModule("g.region", e=10, w=0, s=0, n=10, rows=10, cols=10)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.albedo} = float(0.1 + 0.3 * (row() + col()) / 20.0)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.ndvi} = float(-0.2 + 0.8 * (row() + col()) / 20.0)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.temperature} = float(300.0 + 10.0 * sin((row() + col()) * 0.1))",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.localutctime} = float(12.0 + 1.0 * cos((row() + col()) * 0.2))",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.temperaturedifference2m} = float(3.0 + 2.0 * (row() % 3))",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.emissivity} = float(0.90 + 0.05 * (col() / 10.0))",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.transmissivity} = float(0.65 + 0.1 * sin((row() + col()) * 0.15))",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.dayofyear} = int(180 + 20 * cos((row() + col()) * 0.1))",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.sunzenithangle} = float(35.0 + 10.0 * sin((row() + col()) * 0.2))",
            overwrite=True,
        )
        cls.tmp_rasters.extend(
            [
                cls.albedo,
                cls.ndvi,
                cls.temperature,
                cls.localutctime,
                cls.temperaturedifference2m,
                cls.emissivity,
                cls.transmissivity,
                cls.dayofyear,
                cls.sunzenithangle,
            ]
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region and all generated raster maps."""
        cls.tmp_rasters.append(cls.output)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.tmp_rasters)
        cls.del_temp_region()

    def run_netrad(self, output, **overrides):
        """Helper function to run i.eb.netrad with optional input overrides."""
        args = {
            "albedo": self.albedo,
            "ndvi": self.ndvi,
            "temperature": self.temperature,
            "localutctime": self.localutctime,
            "temperaturedifference2m": self.temperaturedifference2m,
            "emissivity": self.emissivity,
            "transmissivity_singleway": self.transmissivity,
            "dayofyear": self.dayofyear,
            "sunzenithangle": self.sunzenithangle,
            "output": output,
            "overwrite": True,
        }
        args.update(overrides)
        self.runModule("i.eb.netrad", **args)

    def test_netrad_output_validity(self):
        """Verify net radiation calculation produces expected raster with valid statistics."""
        self.run_netrad(self.output)
        self.assertRasterExists(self.output)

        reference_stats = {
            "min": 295.210365,
            "max": 472.952460,
            "mean": 384.149065,
            "stddev": 40.098359,
        }
        self.assertRasterFitsUnivar(self.output, reference_stats, precision=1e-6)

    def test_rn_decreases_with_higher_albedo(self):
        """Confirm inverse relationship between albedo and net radiation."""
        self.runModule("r.mapcalc", expression="albedo_low = 0.1", overwrite=True)
        self.run_netrad("rn_low", albedo="albedo_low")
        self.tmp_rasters.extend(["albedo_low", "rn_low"])
        stats_low = gs.parse_command("r.univar", map="rn_low", format="json")

        self.runModule("r.mapcalc", expression="albedo_high = 0.5", overwrite=True)
        self.run_netrad("rn_high", albedo="albedo_high")
        self.tmp_rasters.extend(["albedo_high", "rn_high"])
        stats_high = gs.parse_command("r.univar", map="rn_high", format="json")

        self.assertGreater(stats_low["mean"], stats_high["mean"])

    def test_rn_increases_with_lower_zenith_angle(self):
        """Validate positive correlation between solar elevation and net radiation."""
        self.runModule("r.mapcalc", expression="zenith_low = 10.0", overwrite=True)
        self.run_netrad("rn_low_zen", sunzenithangle="zenith_low")
        self.tmp_rasters.extend(["zenith_low", "rn_low_zen"])
        stats_low = gs.parse_command("r.univar", map="rn_low_zen", format="json")

        self.runModule("r.mapcalc", expression="zenith_high = 70.0", overwrite=True)
        self.run_netrad("rn_high_zen", sunzenithangle="zenith_high")
        self.tmp_rasters.extend(["zenith_high", "rn_high_zen"])
        stats_high = gs.parse_command("r.univar", map="rn_high_zen", format="json")

        self.assertGreater(stats_low["mean"], stats_high["mean"])

    def test_monotonic_gradient_trend(self):
        """Check expected negative correlation between surface temperature and net radiation."""
        self.runModule(
            "r.mapcalc", expression="linear_temp = 290 + row()", overwrite=True
        )
        self.run_netrad("rn_grad", temperature="linear_temp")
        self.tmp_rasters.append("linear_temp")
        self.runModule("r.mapcalc", expression="row_index = row()", overwrite=True)
        self.tmp_rasters.append("row_index")
        coeffs = gs.parse_command(
            "r.regression.line", mapx="row_index", mapy="rn_grad", flags="g"
        )

        self.assertLess(float(coeffs["b"]), -5.0)
        self.assertLess(float(coeffs["R"]), -0.5)


if __name__ == "__main__":
    test()
