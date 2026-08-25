import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestEvapoPMDetailed(TestCase):
    @classmethod
    def setUpClass(cls):
        """Create test raster maps with random but reproducible values"""

        base_seed = 42
        cls.rasters = [
            ("elevation", "50 + rand(0, 950)", 0),
            ("temperature", "10 + rand(0, 30)", 1),
            ("relativehumidity", "30 + rand(0, 70)", 2),
            ("windspeed", "0.5 + rand(0, 9.5)", 3),
            ("netradiation", "rand(0, 20)", 4),
            ("cropheight", "0.1 + rand(0, 2.9)", 5),
            ("elevation_high", "3000 + rand(0, 1000)", 0),
            ("temp_neg", "-10 - rand(0, 5)", 6),
            ("rh_neg", "95 + rand(0, 5)", 7),
            ("netrad_neg", "-rand(0, 2)", 8),
        ]

        for name, expr, offset in cls.rasters:
            cls.runModule(
                "r.mapcalc",
                expression=f"{name} = {expr}",
                seed=base_seed + offset,
                overwrite=True,
            )

    @classmethod
    def tearDownClass(cls):
        """Cleanup all test maps"""
        cls.runModule(
            "g.remove",
            type="raster",
            pattern="evapo_*,temp_*,rh_*,netrad_*,elevation*,diff_map*",
            flags="f",
        )

    def test_basic_calculation(self):
        """Test core functionality produces valid output range."""

        self.assertModule(
            "i.evapo.pm",
            elevation="elevation",
            temperature="temperature",
            relativehumidity="relativehumidity",
            windspeed="windspeed",
            netradiation="netradiation",
            cropheight="cropheight",
            output="evapo_basic",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            raster="evapo_basic",
            reference={
                "min": 0.000129,
                "max": 6.052911,
                "mean": 2.335533,
                "stddev": 1.470312,
                "sum": 2360290.423755,
            },
            precision=1e-6,
        )

    def test_z_flag_negative_values(self):
        """
        Verify that when negative values exist in the inputs, the module produces
        negative ET values by default, and that using the -z flag clamps these
        negative values to zero.
        """
        self.assertModule(
            "i.evapo.pm",
            elevation="elevation",
            temperature="temp_neg",
            relativehumidity="rh_neg",
            windspeed="windspeed",
            netradiation="netrad_neg",
            cropheight="cropheight",
            output="evapo_negative",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            raster="evapo_negative",
            reference={
                "min": -0.098428,
                "max": 0.003593,
                "mean": -0.031994,
                "stddev": 0.034073,
                "sum": -32333.889996,
            },
            precision=1e-6,
        )

        self.assertModule(
            "i.evapo.pm",
            flags="z",
            elevation="elevation",
            temperature="temp_neg",
            relativehumidity="rh_neg",
            windspeed="windspeed",
            netradiation="netrad_neg",
            cropheight="cropheight",
            output="evapo_zeroed",
            overwrite=True,
        )
        stats = gs.parse_command("r.univar", map="evapo_zeroed", format="json")
        self.assertEqual(
            stats["min"], 0, msg="Minimum value not clamped to zero with -z flag"
        )
        self.assertRasterFitsUnivar(
            raster="evapo_zeroed",
            reference={
                "min": 0,
                "max": 0.003593,
                "mean": 0.000402,
                "stddev": 0.000606,
                "sum": 406.723942,
            },
            precision=1e-6,
        )

    def test_n_flag_night_calculation(self):
        """
        Test that the -n flag correctly computes night-time ET. The ET output for
        night-time should be lower than the day-time ET output.
        """
        self.assertModule(
            "i.evapo.pm",
            elevation="elevation",
            temperature="temperature",
            relativehumidity="relativehumidity",
            windspeed="windspeed",
            netradiation="netradiation",
            cropheight="cropheight",
            output="evapo_day",
            overwrite=True,
        )
        self.assertModule(
            "i.evapo.pm",
            flags="n",
            elevation="elevation",
            temperature="temperature",
            relativehumidity="relativehumidity",
            windspeed="windspeed",
            netradiation="netradiation",
            cropheight="cropheight",
            output="evapo_night",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="diff_night_day = evapo_day - evapo_night",
            overwrite=True,
        )
        stats_diff = gs.parse_command("r.univar", map="diff_night_day", format="json")
        stats_day = gs.parse_command("r.univar", map="evapo_day", format="json")
        stats_night = gs.parse_command("r.univar", map="evapo_night", format="json")
        self.assertGreater(
            stats_day["mean"],
            stats_night["mean"],
            msg="Daytime ET mean is not greater than nighttime ET mean",
        )
        self.assertGreater(
            stats_diff["mean"],
            0.05,
            msg=f"Insignificant mean day-night difference: {stats_diff['mean']}",
        )

    def test_netradiation_dependency(self):
        """
        Verify that a higher net radiation input yields a higher ET output.
        """
        self.runModule("r.mapcalc", expression="netrad_low = 5", overwrite=True)
        self.runModule("r.mapcalc", expression="netrad_high = 20", overwrite=True)
        self.assertModule(
            "i.evapo.pm",
            elevation="elevation",
            temperature="temperature",
            relativehumidity="relativehumidity",
            windspeed="windspeed",
            netradiation="netrad_low",
            cropheight="cropheight",
            output="evapo_netrad_low",
            overwrite=True,
        )
        self.assertModule(
            "i.evapo.pm",
            elevation="elevation",
            temperature="temperature",
            relativehumidity="relativehumidity",
            windspeed="windspeed",
            netradiation="netrad_high",
            cropheight="cropheight",
            output="evapo_netrad_high",
            overwrite=True,
        )
        stats_low = gs.parse_command("r.univar", map="evapo_netrad_low", format="json")
        stats_high = gs.parse_command(
            "r.univar", map="evapo_netrad_high", format="json"
        )
        self.assertGreater(
            stats_high["mean"],
            stats_low["mean"],
            msg="Higher net radiation did not yield higher ET output as expected",
        )

    def test_relativehumidity_dependency(self):
        """
        Test that lower relative humidity (drier air) yields higher ET output
        compared to higher relative humidity.
        """
        self.runModule("r.mapcalc", expression="rh_low = 30", overwrite=True)
        self.runModule("r.mapcalc", expression="rh_high = 70", overwrite=True)
        self.assertModule(
            "i.evapo.pm",
            elevation="elevation",
            temperature="temperature",
            relativehumidity="rh_low",
            windspeed="windspeed",
            netradiation="netradiation",
            cropheight="cropheight",
            output="evapo_rh_low",
            overwrite=True,
        )
        self.assertModule(
            "i.evapo.pm",
            elevation="elevation",
            temperature="temperature",
            relativehumidity="rh_high",
            windspeed="windspeed",
            netradiation="netradiation",
            cropheight="cropheight",
            output="evapo_rh_high",
            overwrite=True,
        )
        stats_low = gs.parse_command("r.univar", map="evapo_rh_low", format="json")
        stats_high = gs.parse_command("r.univar", map="evapo_rh_high", format="json")
        self.assertGreater(
            stats_low["mean"],
            stats_high["mean"],
            msg="Lower relative humidity did not yield higher ET output as expected",
        )

    def test_linearity(self):
        """
        Linearity test: Check that the ET output scales approximately linearly with net radiation.
        For a linear system, the ET computed with an intermediate net radiation (15) should
        be roughly the average of the outputs computed with lower (10) and higher (20) net radiation:
            2 * ET(netrad=15) ≈ ET(netrad=10) + ET(netrad=20)
        """
        self.runModule("r.mapcalc", expression="netrad_10 = 10", overwrite=True)
        self.runModule("r.mapcalc", expression="netrad_15 = 15", overwrite=True)
        self.runModule("r.mapcalc", expression="netrad_20 = 20", overwrite=True)

        for netrad_value, output in [
            (10, "evapo_netrad_10"),
            (15, "evapo_netrad_15"),
            (20, "evapo_netrad_20"),
        ]:
            self.assertModule(
                "i.evapo.pm",
                elevation="elevation",
                temperature="temperature",
                relativehumidity="relativehumidity",
                windspeed="windspeed",
                netradiation=f"netrad_{netrad_value}",
                cropheight="cropheight",
                output=output,
                overwrite=True,
            )
        mean_10 = gs.parse_command("r.univar", map="evapo_netrad_10", format="json")[
            "mean"
        ]
        mean_15 = gs.parse_command("r.univar", map="evapo_netrad_15", format="json")[
            "mean"
        ]
        mean_20 = gs.parse_command("r.univar", map="evapo_netrad_20", format="json")[
            "mean"
        ]
        self.assertAlmostEqual(
            2 * mean_15,
            mean_10 + mean_20,
            places=6,
            msg="ET output does not scale linearly with net radiation as expected",
        )


if __name__ == "__main__":
    test()
