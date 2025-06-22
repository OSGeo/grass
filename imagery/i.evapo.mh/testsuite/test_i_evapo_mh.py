import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import math


class TestEvapotranspirationMH(TestCase):
    """Regression tests for i.evapo.mh module"""

    tmp_rasters = []
    netradiation_diurnal = "netrad"
    average_temperature = "avg_temp"
    minimum_temperature = "min_temp"
    maximum_temperature = "max_temp"
    precipitation = "precipitation"
    output_map = "et_output"

    @classmethod
    def setUpClass(cls):
        """Setup input rasters and configure test environment."""
        seed = 43
        cls.use_temp_region()
        gs.run_command("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.netradiation_diurnal} = rand(0, 160)",
            seed=seed,
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.minimum_temperature} = rand(0, 15)",
            seed=seed,
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.maximum_temperature} = rand(25, 40)",
            seed=seed,
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.average_temperature} = ({cls.minimum_temperature}+{cls.maximum_temperature})/2",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.precipitation} = rand(50, 130)",
            seed=seed,
            overwrite=True,
        )
        cls.tmp_rasters.extend(
            [
                cls.netradiation_diurnal,
                cls.average_temperature,
                cls.minimum_temperature,
                cls.maximum_temperature,
                cls.precipitation,
            ]
        )

    @classmethod
    def tearDownClass(cls):
        """Remove generated rasters and reset test environment."""
        gs.run_command("g.remove", type="raster", name=cls.tmp_rasters, flags="f")
        cls.del_temp_region()

    def create_raster(self, name, expression, seed=None):
        """Create temporary raster map and register it for cleanup."""
        self.runModule(
            "r.mapcalc",
            expression=f"{name} = {expression}",
            seed=seed,
            overwrite=True,
        )
        self.tmp_rasters.append(name)

    def run_evapo(self, output, flags="", overwrite=True, **kwargs):
        """Execute the i.evapo.mh module with standardized parameters."""
        self.assertModule(
            "i.evapo.mh",
            flags=flags,
            output=output,
            overwrite=overwrite,
            **kwargs,
        )
        self.tmp_rasters.append(output)

    def test_z_flag_negative_values(self):
        """Verify -z flag clamps negative output values to zero."""
        self.create_raster("netrad_z", "10")
        self.create_raster("min_temp_z", "0")
        self.create_raster("max_temp_z", "10")
        self.create_raster("avg_temp_z", "(min_temp_z + max_temp_z)/2")

        self.run_evapo(
            flags="z",
            netradiation_diurnal="netrad_z",
            average_temperature="avg_temp_z",
            minimum_temperature="min_temp_z",
            maximum_temperature="max_temp_z",
            precipitation=self.precipitation,
            output=self.output_map,
        )
        stats = gs.parse_command(
            "r.univar", map=self.output_map, flags="g", format="json"
        )
        self.assertGreaterEqual(stats["min"], 0)

    def test_h_flag_precipitation_ignored(self):
        """Confirm -h flag makes precipitation input irrelevant."""
        self.run_evapo(
            flags="h",
            netradiation_diurnal=self.netradiation_diurnal,
            average_temperature=self.average_temperature,
            minimum_temperature=self.minimum_temperature,
            maximum_temperature=self.maximum_temperature,
            precipitation=self.precipitation,
            output=f"{self.output_map}_h",
        )
        self.run_evapo(
            flags="h",
            netradiation_diurnal=self.netradiation_diurnal,
            average_temperature=self.average_temperature,
            minimum_temperature=self.minimum_temperature,
            maximum_temperature=self.maximum_temperature,
            output=f"{self.output_map}_h_noprecip",
        )
        self.assertRastersNoDifference(
            f"{self.output_map}_h",
            f"{self.output_map}_h_noprecip",
            precision=1e-6,
        )

    def test_s_flag_different_output(self):
        """Check -s flag produces different results from default method."""
        self.run_evapo(
            netradiation_diurnal=self.netradiation_diurnal,
            average_temperature=self.average_temperature,
            minimum_temperature=self.minimum_temperature,
            maximum_temperature=self.maximum_temperature,
            precipitation=self.precipitation,
            output=f"{self.output_map}_default",
        )
        self.run_evapo(
            flags="s",
            netradiation_diurnal=self.netradiation_diurnal,
            average_temperature=self.average_temperature,
            minimum_temperature=self.minimum_temperature,
            maximum_temperature=self.maximum_temperature,
            precipitation=self.precipitation,
            output=f"{self.output_map}_s",
        )
        self.create_raster(
            "diff_map", f"{self.output_map}_default - {self.output_map}_s"
        )
        stats = {"min": 0, "max": 2.276227, "mean": 0.985087}
        self.assertRasterFitsUnivar(raster="diff_map", reference=stats, precision=1e-6)

    def test_hargreaves_formula(self):
        """Validate Hargreaves formula implementation matches reference."""
        self.create_raster("netrad_test", "0.1446759")
        self.create_raster("tmin_test", "10")
        self.create_raster("tmax_test", "30")
        self.create_raster("tavg_test", "20")

        ra_input_w = 0.1446759
        tmin = 10
        tmax = 30
        tavg = 20

        ra_mj = ra_input_w * 0.0864
        td = tmax - tmin

        expected_et = 0.0023 * 0.408 * ra_mj * (tavg + 17.8) * math.sqrt(td)

        self.run_evapo(
            flags="h",
            netradiation_diurnal="netrad_test",
            average_temperature="tavg_test",
            minimum_temperature="tmin_test",
            maximum_temperature="tmax_test",
            output="et_const",
        )
        stats = gs.parse_command("r.univar", map="et_const", flags="g", format="json")
        self.assertAlmostEqual(stats["mean"], expected_et)


if __name__ == "__main__":
    test()
