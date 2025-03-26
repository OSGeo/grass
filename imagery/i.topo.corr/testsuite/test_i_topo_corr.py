import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestITopoCorr(TestCase):
    """Regression tests for i.topo.corr topographic correction."""

    input_reflectance = "test_refl"
    basemap = "elevation"
    output_corrected = "corrected"
    illumination_map = "illumination"
    zenith = 45.0
    azimuth = 315.0

    def setUp(self):
        """Set up synthetic test data."""
        self.use_temp_region()
        self.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        self.runModule(
            "r.mapcalc",
            expression=f"{self.basemap} = 100 + row()*5 + col()*3 + sin(row()*2) + cos(col()*2)",
            overwrite=True,
        )
        self.runModule(
            "r.slope.aspect",
            elevation=self.basemap,
            slope="slope",
            aspect="aspect",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_reflectance} = 0.1 + (row() + col())/200 + rand(0, 0.05)",
            seed=42,
            overwrite=True,
        )

    def tearDown(self):
        """Clean up generated data."""
        self.runModule(
            "g.remove",
            type="raster",
            flags="f",
            name=[
                self.basemap,
                "slope",
                "aspect",
                self.input_reflectance,
                self.output_corrected,
                self.illumination_map,
            ],
        )
        self.runModule(
            "g.remove", type="raster", flags="f", pattern="corr_*,scaled_refl_*"
        )
        self.del_temp_region()

    def _create_illumination_map(self):
        """Helper to generate illumination map."""
        self.runModule(
            "i.topo.corr",
            flags="i",
            basemap=self.basemap,
            output=self.illumination_map,
            zenith=self.zenith,
            azimuth=self.azimuth,
            overwrite=True,
        )

    def test_illumination_model(self):
        """Verify illumination model stats."""
        self._create_illumination_map()
        self.assertRasterFitsUnivar(
            raster=self.illumination_map,
            reference={
                "mean": 0.794546,
                "sum": 38.13825,
                "n": 48,
                "null_cells": 52,
            },
            precision=1e-6,
        )

    def test_cosine_method(self):
        """Test cosine correction method."""
        self._create_illumination_map()
        output_name = "corr_cosine"
        self.assertModule(
            "i.topo.corr",
            basemap=self.illumination_map,
            input=self.input_reflectance,
            output=output_name,
            zenith=self.zenith,
            method="cosine",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            raster=f"{output_name}.{self.input_reflectance}",
            reference={
                "mean": 0.1089169,
                "sum": 5.228013,
                "min": 0.0894596,
                "max": 0.130911,
                "stddev": 0.0120903,
            },
            precision=1e-6,
        )

    def test_cfactor_method(self):
        """Test c-factor correction method."""
        self._create_illumination_map()
        output_name = "corr_c-factor"
        self.assertModule(
            "i.topo.corr",
            basemap=self.illumination_map,
            input=self.input_reflectance,
            output=output_name,
            zenith=self.zenith,
            method="c-factor",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            raster=f"{output_name}.{self.input_reflectance}",
            reference={
                "mean": 4.642691,
                "sum": 222.849179,
                "min": 3.866187,
                "max": 5.571429,
                "stddev": 0.507151,
            },
            precision=1e-6,
        )

    def test_percent_method(self):
        """Test percent correction method."""
        self._create_illumination_map()
        output_name = "corr_percent"
        self.assertModule(
            "i.topo.corr",
            basemap=self.illumination_map,
            input=self.input_reflectance,
            output=output_name,
            zenith=self.zenith,
            method="percent",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            raster=f"{output_name}.{self.input_reflectance}",
            reference={
                "mean": 0.136397,
                "sum": 6.547056,
                "min": 0.112033,
                "max": 0.163939,
                "stddev": 0.0151399,
            },
            precision=1e-6,
        )

    def test_scale_flag(self):
        """Verify the -s flag preserves range and colors."""
        self._create_illumination_map()
        self.assertModule(
            "i.topo.corr",
            basemap=self.illumination_map,
            input=self.input_reflectance,
            output=self.output_corrected,
            zenith=self.zenith,
            method="c-factor",
            flags="s",
            overwrite=True,
        )
        output_map = f"{self.output_corrected}.{self.input_reflectance}"

        orig_stats = gs.parse_command(
            "r.univar", map=self.input_reflectance, format="json"
        )
        corr_stats = gs.parse_command("r.univar", map=output_map, format="json")
        self.assertAlmostEqual(
            orig_stats[0]["min"],
            corr_stats[0]["min"],
            delta=1e-2,
            msg="Min value not preserved.",
        )
        self.assertAlmostEqual(
            orig_stats[0]["max"],
            corr_stats[0]["max"],
            delta=1e-2,
            msg="Max value not preserved",
        )

        orig_colors = gs.parse_command("r.colors.out", map=self.input_reflectance)
        corr_colors = gs.parse_command("r.colors.out", map=output_map)
        self.assertEqual(orig_colors, corr_colors, "Color rules not copied")

    def test_linearity(self):
        """Verify linear scaling."""
        self._create_illumination_map()
        scales = [0.5, 1.0, 2.0]
        results = []
        for scale in scales:
            scaled_input = f"scaled_refl_{scale}"
            self.runModule(
                "r.mapcalc",
                expression=f"{scaled_input} = {scale} * {self.input_reflectance}",
                overwrite=True,
            )
            output_name = f"corr_linear_{scale}"
            self.assertModule(
                "i.topo.corr",
                basemap=self.illumination_map,
                input=scaled_input,
                output=output_name,
                zenith=self.zenith,
                method="cosine",
                overwrite=True,
            )
            stats = gs.parse_command(
                "r.univar", map=f"{output_name}.{scaled_input}", format="json"
            )
            results.append(stats[0]["mean"])
        self.assertAlmostEqual(
            results[1] / results[0], scales[1] / scales[0], delta=1e-6
        )
        self.assertAlmostEqual(
            results[2] / results[1], scales[2] / scales[1], delta=1e-6
        )


if __name__ == "__main__":
    test()
