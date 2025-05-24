import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestEvapoTimeDetailed(TestCase):
    @classmethod
    def setUpClass(cls):
        """Create test raster maps with random but reproducible values"""
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)

        base_seed = 42
        cls.eto_doy_min = 120.0
        cls.start_period = 120.0
        cls.end_period = 125.0
        cls.eta_maps = []
        for i in range(1, 3):
            eta_name = f"eta_{i}"
            cls.runModule(
                "r.mapcalc",
                expression=f"{eta_name} = 10 + rand(0, 20)",
                seed=base_seed + i,
                overwrite=True,
            )
            cls.eta_maps.append(eta_name)
        cls.eta_doy_maps = []
        for i in range(1, 3):
            eta_doy_name = f"eta_doy_{i}"
            doy = cls.start_period + (i - 1) * 2
            cls.runModule(
                "r.mapcalc", expression=f"{eta_doy_name} = {doy}", overwrite=True
            )
            cls.eta_doy_maps.append(eta_doy_name)
        cls.eto_maps = []
        num_eto_days = int(cls.end_period - cls.eto_doy_min) + 1
        for doy_offset in range(num_eto_days):
            eto_name = f"eto_{doy_offset}"
            cls.runModule(
                "r.mapcalc",
                expression=f"{eto_name} = 15 + rand(1, 25)",
                seed=base_seed + doy_offset,
                overwrite=True,
            )
            cls.eto_maps.append(eto_name)

    @classmethod
    def tearDownClass(cls):
        """Remove all temporary raster maps."""
        cls.runModule(
            "g.remove",
            type="raster",
            pattern="evapo_time*,eta_*,eta_doy_*,eto_*",
            flags="f",
        )

    def test_cumulative_et_statistics(self):
        """Test that the module produces cumulative ET output with expected summary statistics."""
        self.assertModule(
            "i.evapo.time",
            eta=",".join(self.eta_maps),
            eta_doy=",".join(self.eta_doy_maps),
            eto=",".join(self.eto_maps),
            eto_doy_min=self.eto_doy_min,
            start_period=self.start_period,
            end_period=self.end_period,
            output="evapo_time_basic",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            raster="evapo_time_basic",
            reference={
                "min": 41.324325,
                "max": 157.5,
                "mean": 81.832618,
                "stddev": 24.378427,
            },
            precision=1e-6,
        )

    def test_time_range_effects(self):
        """
        Verify that extending the time period of the analysis increases the cumulative ET.
        """
        self.assertModule(
            "i.evapo.time",
            eta=",".join(self.eta_maps),
            eta_doy=",".join(self.eta_doy_maps),
            eto=",".join(self.eto_maps),
            eto_doy_min=self.eto_doy_min,
            start_period=120.0,
            end_period=122.0,
            output="evapo_time_short",
            overwrite=True,
        )
        self.assertModule(
            "i.evapo.time",
            eta=",".join(self.eta_maps),
            eta_doy=",".join(self.eta_doy_maps),
            eto=",".join(self.eto_maps),
            eto_doy_min=self.eto_doy_min,
            start_period=120.0,
            end_period=125.0,
            output="evapo_time_long",
            overwrite=True,
        )
        stats_short = gs.parse_command(
            "r.univar", map="evapo_time_short", flags="g", format="json"
        )
        stats_long = gs.parse_command(
            "r.univar", map="evapo_time_long", flags="g", format="json"
        )
        self.assertGreater(
            stats_long["sum"],
            stats_short["sum"],
            msg="Longer time period did not yield a higher cumulative ET.",
        )

    def test_linear_scaling_with_eto1(self):
        """
        Verify cumulative ET scales linearly with ETo when ETrF is constant.
        """
        self.runModule("r.mapcalc", expression="eta_doy_ctl1 = 120", overwrite=True)
        self.runModule("r.mapcalc", expression="eta_doy_ctl2 = 122", overwrite=True)

        eto_scaling = {"low": 1, "mid": 2, "high": 3}
        base_eto_value = 10

        for suffix, factor in eto_scaling.items():
            for doy in range(120, 126):
                day_offset = doy - 120
                eto_value = factor * (base_eto_value + day_offset)
                self.runModule(
                    "r.mapcalc",
                    expression=f"eto_{suffix}_{doy} = {eto_value}",
                    overwrite=True,
                )
            self.runModule(
                "r.mapcalc",
                expression=f"eta_ctl1_{suffix} = 0.8 * eto_{suffix}_120",
                overwrite=True,
            )
            self.runModule(
                "r.mapcalc",
                expression=f"eta_ctl2_{suffix} = 0.8 * eto_{suffix}_122",
                overwrite=True,
            )

        results = {}
        for suffix in eto_scaling.keys():
            eto_maps = [f"eto_{suffix}_{doy}" for doy in range(120, 126)]
            self.assertModule(
                "i.evapo.time",
                eta=f"eta_ctl1_{suffix},eta_ctl2_{suffix}",
                eta_doy="eta_doy_ctl1,eta_doy_ctl2",
                eto=",".join(eto_maps),
                eto_doy_min=120.0,
                start_period=120.0,
                end_period=125.0,
                output=f"evapo_time_{suffix}",
                overwrite=True,
            )
            stats = gs.parse_command(
                "r.univar", map=f"evapo_time_{suffix}", flags="g", format="json"
            )
            results[suffix] = stats["sum"]
        self.assertAlmostEqual(
            results["mid"] / results["low"],
            2.0,
            places=6,
            msg="Mid cumulative ET should be double Low",
        )
        self.assertAlmostEqual(
            results["high"] / results["low"],
            3.0,
            places=6,
            msg="High cumulative ET should be triple Low",
        )


if __name__ == "__main__":
    test()
