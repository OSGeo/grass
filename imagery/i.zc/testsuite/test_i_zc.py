import grass.script as gs
from grass.script import core as grass
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIZC(TestCase):
    """Regression tests for the i.zc GRASS GIS module."""

    input_raster = "input_raster"
    output_raster = "output_raster"

    @classmethod
    def setUpClass(cls):
        """Set up an input raster and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=16, s=0, e=16, w=0, rows=16, cols=16)
        cls.temp_rasters = []
        cls.runModule("r.mapcalc", expression=f"{cls.input_raster} = col() + row()")
        cls.temp_rasters.append(cls.input_raster)

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data and reset the region."""
        cls.temp_rasters.append(cls.output_raster)
        cls.runModule("g.remove", type="raster", name=cls.temp_rasters, flags="f")
        cls.del_temp_region()

    def test_zero_crossing_pattern(self):
        """Test zero-crossing detection on a vertical split pattern."""
        split = "vertical_split"
        self.runModule("r.mapcalc", expression=f"{split} = if(col() < 8, 0, 1)")
        self.temp_rasters.append(split)

        self.assertModule(
            "i.zc", input=split, output=self.output_raster, overwrite=True
        )

        stats = {
            "n": 256,
            "null_cells": 0,
            "mean": 0.109375,
            "stddev": 0.312109,
            "sum": 28,
        }

        self.assertRasterFitsUnivar(self.output_raster, reference=stats, precision=1e-6)

    def test_orientation_effect(self):
        """Test the effect of different orientation bin counts on zero-crossing detection."""
        expected_category_counts = {
            8: {"0": 229, "2": 4, "4": 6, "6": 11, "8": 6},
            16: {"0": 229, "3": 4, "6": 4, "7": 2, "11": 11, "15": 2, "16": 4},
        }
        for orientations in [8, 16]:
            output = f"output_raster_{orientations}"
            self.assertModule(
                "i.zc",
                input=self.input_raster,
                output=output,
                orientations=orientations,
                overwrite=True,
            )
            self.assertRasterExists(output)
            self.temp_rasters.append(output)

            stats_str = grass.read_command("r.stats", flags="c", input=output)
            actual_category_counts = {}
            for line in stats_str.strip().splitlines():
                parts = line.split()
                if len(parts) == 2:
                    category, count = parts
                    actual_category_counts[category] = int(count)

            expected = expected_category_counts[orientations]
            self.assertEqual(actual_category_counts, expected)

    def test_threshold_effect(self):
        """
        Test the impact of threshold values on edge detection sensitivity.
        Ensuring that the lower threshold detects more edges.
        """
        self.runModule("g.region", n=16, s=0, e=16, w=0, rows=32, cols=32)
        complex_input_raster = "complex_input"
        self.runModule(
            "r.mapcalc",
            expression=f"{complex_input_raster} = 50 + 20*sin((col()-8)*3.1416/8) + 20*cos((row()-8)*3.1416/8)",
            overwrite=True,
        )
        self.temp_rasters.append(complex_input_raster)

        low_thresh = "output_thresh_low"
        self.assertModule(
            "i.zc",
            input=complex_input_raster,
            output=low_thresh,
            threshold=0.1,
            overwrite=True,
        )

        high_thresh = "output_thresh_high"
        self.assertModule(
            "i.zc",
            input=complex_input_raster,
            output=high_thresh,
            threshold=5,
            overwrite=True,
        )
        self.temp_rasters.extend([low_thresh, high_thresh])

        low_stats_str = gs.parse_command("r.univar", map=low_thresh, format="json")
        high_stats_str = gs.parse_command("r.univar", map=high_thresh, format="json")

        low_edge_count = low_stats_str[0]["sum"]
        high_edge_count = high_stats_str[0]["sum"]

        self.assertGreater(low_edge_count, high_edge_count)
        self.runModule("g.region", n=16, s=0, e=16, w=0, rows=16, cols=16)

    def test_width_effect(self):
        """
        Test the effect of edge detection width on sensitivity.
        Ensure that a smaller width detects more edges.
        """
        self.runModule("g.region", n=16, s=0, e=16, w=0, rows=512, cols=512)
        gradual_increase_raster = "gradual_increase"
        self.runModule(
            "r.mapcalc",
            expression=(
                f"{gradual_increase_raster} = "
                "if(col() < 256, 0, 100 + (col() - 256) * (100.0 / 256))"
            ),
            overwrite=True,
        )
        self.temp_rasters.append(gradual_increase_raster)
        low_width = "output_width_low"
        self.runModule(
            "i.zc",
            input=gradual_increase_raster,
            output=low_width,
            width=1,
            overwrite=True,
        )

        high_width = "output_width_high"
        self.runModule(
            "i.zc",
            input=gradual_increase_raster,
            output=high_width,
            width=16,
            overwrite=True,
        )
        self.temp_rasters.extend([low_width, high_width])

        low_stats_str = gs.parse_command("r.univar", map=low_width, format="json")
        high_stats_str = gs.parse_command("r.univar", map=high_width, format="json")

        low_edge_count = low_stats_str[0]["sum"]
        high_edge_count = high_stats_str[0]["sum"]

        self.assertGreater(low_edge_count, high_edge_count)
        self.runModule("g.region", n=16, s=0, e=16, w=0, rows=16, cols=16)


if __name__ == "__main__":
    test()
