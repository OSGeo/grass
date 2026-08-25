import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestEbEta(TestCase):
    """Regression tests for i.eb.eta module"""

    tmp_rasters = []
    netradiationdiurnal = "netrad"
    evaporativefraction = "evapfrac"
    temperature = "temp"
    output = "eb_eta_output"

    @classmethod
    def setUpClass(cls):
        """Create input raster maps and set up the region."""
        seed = 42
        cls.use_temp_region()
        gs.run_command("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

        cls._create_raster(cls.netradiationdiurnal, "rand(0, 160)", seed)
        cls._create_raster(
            cls.evaporativefraction, "0.5 + 0.1 * row() + 0.1 * col()", seed
        )
        cls._create_raster(cls.temperature, "rand(280, 340)", seed)

    @classmethod
    def tearDownClass(cls):
        """Remove all temporary rasters created by the tests and restore region."""
        cls.del_temp_region()
        cls.tmp_rasters.append(cls.output)
        gs.run_command("g.remove", type="raster", name=cls.tmp_rasters, flags="f")

    @classmethod
    def _create_raster(cls, name, expression, seed=None):
        """Helper function to create rasters."""
        if seed is not None:
            cls.runModule(
                "r.mapcalc",
                expression=f"{name} = {expression}",
                seed=seed,
                overwrite=True,
            )
        else:
            cls.runModule(
                "r.mapcalc", expression=f"{name} = {expression}", overwrite=True
            )
        cls.tmp_rasters.append(name)

    def _run_eta(self, netrad, evapfrac, temp, output):
        """Helper function to run i.eb.eta."""
        self.assertModule(
            "i.eb.eta",
            netradiationdiurnal=netrad,
            evaporativefraction=evapfrac,
            temperature=temp,
            output=output,
            overwrite=True,
        )
        self.assertRasterExists(output)
        self.tmp_rasters.append(output)

    def _get_univar_stats(self, raster):
        """Helper to get univariate statistics"""
        return gs.parse_command("r.univar", map=raster, format="json")

    def test_eta_model_output(self):
        """Basic test to verify output of i.eb.eta."""
        self._run_eta(
            self.netradiationdiurnal,
            self.evaporativefraction,
            self.temperature,
            self.output,
        )

        reference_stats = {
            "min": 0.0,
            "max": 12.764194,
            "mean": 4.724633,
            "stddev": 2.849859,
        }

        self.assertRasterFitsUnivar(self.output, reference_stats, precision=1e-6)

    def test_eta_numerical_stability(self):
        """Ensure numerical stability with constant net radiation and temperature gradient."""
        netrad_num = f"{self.netradiationdiurnal}_num"
        temp_num = f"{self.temperature}_num"
        output_num = f"{self.output}_num"

        self._create_raster(netrad_num, "120")
        self._create_raster(temp_num, "280.0 + 0.5 * row()")

        self._run_eta(netrad_num, self.evaporativefraction, temp_num, output_num)

        stats = self._get_univar_stats(output_num)
        self.assertGreaterEqual(stats["min"], 0.0, msg="Negative ET values found")

        corr = gs.parse_command(
            "r.regression.line",
            mapx=self.evaporativefraction,
            mapy=output_num,
            flags="g",
        )

        self.assertAlmostEqual(
            float(corr["R"]),
            1.0,
            delta=0.01,
            msg="Output should be strongly correlated with evaporative fraction",
        )

    def test_netradiation_dependency(self):
        """Test if ET is proportional to net radiation for constant temperature and evapfrac."""
        temp_const = f"{self.temperature}_const"
        self._create_raster(temp_const, "300")

        netrad_low = f"{self.netradiationdiurnal}_low"
        netrad_high = f"{self.netradiationdiurnal}_high"
        self._create_raster(netrad_low, "50")
        self._create_raster(netrad_high, "150")

        evap_const = f"{self.evaporativefraction}_const"
        self._create_raster(evap_const, "0.7")

        output_low = f"{self.output}_netrad_low"
        output_high = f"{self.output}_netrad_high"

        self._run_eta(netrad_low, evap_const, temp_const, output_low)
        self._run_eta(netrad_high, evap_const, temp_const, output_high)

        stats_low = self._get_univar_stats(output_low)
        stats_high = self._get_univar_stats(output_high)

        # 3x net radiation should give ~3x ET
        ratio_netrad = 150.0 / 50.0
        ratio_et = float(stats_high["mean"]) / float(stats_low["mean"])
        self.assertAlmostEqual(
            ratio_et,
            ratio_netrad,
            delta=0.05,
            msg="ET not proportional to net radiation as expected",
        )

    def test_evaporative_fraction_dependency(self):
        """Test if ET scales proportionally with evaporative fraction for fixed netrad and temp."""
        temp_const = f"{self.temperature}_const"
        netrad_const = f"{self.netradiationdiurnal}_const"
        self._create_raster(temp_const, "300")
        self._create_raster(netrad_const, "100")

        evap_low = f"{self.evaporativefraction}_low"
        evap_high = f"{self.evaporativefraction}_high"
        self._create_raster(evap_low, "0.3")
        self._create_raster(evap_high, "0.9")

        output_low = f"{self.output}_ef_low"
        output_high = f"{self.output}_ef_high"

        self._run_eta(netrad_const, evap_low, temp_const, output_low)
        self._run_eta(netrad_const, evap_high, temp_const, output_high)

        stats_low = gs.parse_command("r.univar", map=output_low, flags="g")
        stats_high = gs.parse_command("r.univar", map=output_high, flags="g")

        ratio_ef = 0.9 / 0.3
        ratio_et = float(stats_high["mean"]) / float(stats_low["mean"])
        self.assertAlmostEqual(
            ratio_et,
            ratio_ef,
            delta=0.05,
            msg="ET not proportional to evaporative fraction as expected",
        )


if __name__ == "__main__":
    test()
