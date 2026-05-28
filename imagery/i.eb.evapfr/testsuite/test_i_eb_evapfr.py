import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestEvaporativeFraction(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set the GRASS region and generate synthetic input rasters."""
        cls.input_prefix = "input_"
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1, flags="p")
        cls.runModule(
            "r.mapcalc", expression="input_netrad = 500 + row() * 10", overwrite=True
        )
        cls.runModule(
            "r.mapcalc",
            expression="input_soilheat = 0.1 * input_netrad",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc", expression="input_sensible = 50 + row() * 2", overwrite=True
        )

    @classmethod
    def tearDownClass(cls):
        """Remove all synthetic maps created."""
        cls.runModule(
            "g.remove",
            type="raster",
            pattern="input_*,evapfr_*,soilmoist_*,netrad_*,sensible_*,*more,*less,*scaled",
            flags="f",
        )

    def compute_evapfr_output(self):
        """Helper function to execute the module"""
        self.assertModule(
            "i.eb.evapfr",
            netradiation="input_netrad",
            soilheatflux="input_soilheat",
            sensibleheatflux="input_sensible",
            evaporativefraction="evapfr_output",
            overwrite=True,
        )

    def test_evapfr_stats(self):
        """Test that evaporative fraction output matches expected statistics."""
        self.compute_evapfr_output()
        self.assertRasterFitsUnivar(
            "evapfr_output",
            reference={
                "min": 0.870370,
                "max": 0.886710,
                "mean": 0.878147,
                "stddev": 0.005210,
            },
            precision=1e-6,
        )

    def test_m_flag(self):
        """Test that -m flag generates soil moisture output."""
        self.assertModule(
            "i.eb.evapfr",
            flags="m",
            netradiation="input_netrad",
            soilheatflux="input_soilheat",
            sensibleheatflux="input_sensible",
            evaporativefraction="evapfr_output",
            soilmoisture="soilmoist_output",
            overwrite=True,
        )

        self.assertRasterFitsUnivar(
            "soilmoist_output",
            reference={
                "min": 0.733844,
                "max": 0.762865,
                "mean": 0.747573,
                "stddev": 0.009252,
            },
            precision=1e-6,
        )

    def test_evapfr_decreases_with_increased_sensible_heat(self):
        """Verify that increasing sensible heat results in lower evaporative fraction."""
        self.compute_evapfr_output()
        self.runModule(
            "r.mapcalc",
            expression="sensible_high = input_sensible + 100",
            overwrite=True,
        )

        self.assertModule(
            "i.eb.evapfr",
            netradiation="input_netrad",
            soilheatflux="input_soilheat",
            sensibleheatflux="sensible_high",
            evaporativefraction="evapfr_less",
            overwrite=True,
        )

        stats_base = gs.parse_command("r.univar", map="evapfr_output", flags="g")
        stats_less = gs.parse_command("r.univar", map="evapfr_less", flags="g")
        self.assertGreater(
            stats_base["mean"],
            stats_less["mean"],
            msg="Evap. fraction did not decrease with increased sensible heat.",
        )

    def test_evapfr_increases_with_increased_net_radiation(self):
        """Verify that increasing net radiation results in higher evaporative fraction."""
        self.compute_evapfr_output()
        self.runModule(
            "r.mapcalc", expression="netrad_high = input_netrad + 100", overwrite=True
        )

        self.assertModule(
            "i.eb.evapfr",
            netradiation="netrad_high",
            soilheatflux="input_soilheat",
            sensibleheatflux="input_sensible",
            evaporativefraction="evapfr_more",
            overwrite=True,
        )

        stats_base = gs.parse_command("r.univar", map="evapfr_output", flags="g")
        stats_more = gs.parse_command("r.univar", map="evapfr_more", flags="g")
        self.assertGreater(
            stats_more["mean"],
            stats_base["mean"],
            msg="Evap. fraction did not increase with increased net radiation.",
        )

    def test_evapfr_invariance_to_linear_energy_scaling(self):
        """Ensure that linearly scaling all energy flux inputs does not affect evaporative fraction."""
        self.compute_evapfr_output()
        self.runModule(
            "r.mapcalc", expression="netrad_scaled = 2 * input_netrad", overwrite=True
        )
        self.runModule(
            "r.mapcalc",
            expression="soilheat_scaled = 2 * input_soilheat",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="sensible_scaled = 2 * input_sensible",
            overwrite=True,
        )

        self.assertModule(
            "i.eb.evapfr",
            netradiation="netrad_scaled",
            soilheatflux="soilheat_scaled",
            sensibleheatflux="sensible_scaled",
            evaporativefraction="evapfr_scaled",
            overwrite=True,
        )

        stats_base = gs.parse_command("r.univar", map="evapfr_output", flags="g")
        stats_scaled = gs.parse_command("r.univar", map="evapfr_scaled", flags="g")
        self.assertAlmostEqual(
            stats_base["mean"],
            stats_scaled["mean"],
            places=6,
            msg="Linearly scaled inputs changed evaporative fraction.",
        )


if __name__ == "__main__":
    test()
