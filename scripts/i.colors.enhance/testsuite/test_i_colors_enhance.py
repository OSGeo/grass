import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestColorsEnhance(TestCase):
    """Regression test suite for i.colors.enhance module in GRASS."""

    @classmethod
    def setUpClass(cls):
        """Generate test raster maps simulating RGB bands."""
        cls.rgb_maps = ["test_red", "test_green", "test_blue"]
        expressions = [
            "test_red = row()",
            "test_green = col()",
            "test_blue = row() + col()",
        ]
        for expr in expressions:
            cls.runModule("r.mapcalc", expression=expr, overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Clean up all raster maps created for testing."""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.rgb_maps)

    def setUp(self):
        """Re-apply the default grey255 color table to all test maps
        before each test case to ensure consistent starting conditions."""
        for m in self.rgb_maps:
            self._set_color_table(m, "grey255")

    def _set_color_table(self, mapname, color):
        """Assign a specified color table to a raster map."""
        gs.run_command("r.colors", map=mapname, color=color, quiet=True)

    def _get_color_table(self, mapname):
        """Get the color table of the raster map as a string."""
        return gs.read_command("r.colors.out", map=mapname)

    def _get_color_table_minmax(self, map_name):
        """
        Extract the minimum and maximum values from a raster's color table.
        """
        lines = self._get_color_table(map_name).splitlines()
        min_val = float(lines[0].split()[0])
        max_val = float(
            next(
                line
                for line in reversed(lines)
                if not line.startswith(("nv", "default"))
            ).split()[0]
        )
        return min_val, max_val

    def _run_colors_enhance(self, **kwargs):
        """Run the i.colors.enhance module on the RGB bands
        with the provided flags and parameters.
        """
        return self.assertModule(
            "i.colors.enhance",
            red="test_red",
            green="test_green",
            blue="test_blue",
            **kwargs,
        )

    def test_default_enhancement_applies_new_colors(self):
        """Verify that default enhancement replaces the grey255 color tables."""
        self._run_colors_enhance()
        for m in self.rgb_maps:
            colors = self._get_color_table(m)
            self.assertNotIn(
                "grey255",
                colors,
                msg=f"{m} still has grey255 color table, enhancement failed.",
            )

    def test_full_range_flag_sets_grey(self):
        """Verify that the -f (full-range) flag applies the standard grey gradient color table."""
        self._run_colors_enhance(flags="f")
        grey_table = self._get_color_table("test_red")

        lines = [
            line
            for line in grey_table.strip().splitlines()
            if not line.startswith(("nv", "default"))
        ]
        self.assertGreaterEqual(len(lines), 2, "Expected at least two color entries.")

        first_val, first_rgb = lines[0].split()
        last_val, last_rgb = lines[-1].split()

        self.assertEqual(first_rgb, "0:0:0", msg="Min color is not black (0:0:0)")
        self.assertEqual(
            last_rgb, "255:255:255", msg="Max color is not white (255:255:255)"
        )

    def test_reset_flag_restores_grey255(self):
        """Verify that the -r (reset) flag restores the grey255 color table."""
        self._set_color_table("test_red", "grey")
        self._run_colors_enhance(flags="r")

        result = self._get_color_table("test_red")

        self._set_color_table("test_green", "grey255")
        expected = self._get_color_table("test_green")

        self.assertEqual(
            result.strip(),
            expected.strip(),
            msg="Reset flag did not restore grey255 color table correctly.",
        )

    def test_preserve_relative_scaling(self):
        """
        Verify that the -p (preserve relative scaling) flag
        maintains proportional scaling across bands.
        """
        self._run_colors_enhance(flags="p")

        red_min, red_max = self._get_color_table_minmax("test_red")
        red_range = red_max - red_min

        self.assertGreater(
            red_range, 0, "Red band has zero range, cannot compare scaling."
        )

        for band in self.rgb_maps:
            band_min, band_max = self._get_color_table_minmax(band)
            band_range = band_max - band_min

            ratio = band_range / red_range

            self.assertGreater(
                ratio,
                0.5,
                msg=f"{band} range too compressed relative to red (ratio={ratio:.2f}).",
            )
            self.assertLess(
                ratio,
                2.0,
                msg=f"{band} range too expanded relative to red (ratio={ratio:.2f}).",
            )

    def test_serial_processing_mode(self):
        """Verify that the -s (serial processing) flag enhances colors correctly."""
        self._run_colors_enhance(flags="s")
        for m in self.rgb_maps:
            colors = self._get_color_table(m)
            self.assertNotIn(
                "grey255",
                colors,
                msg=f"{m} still has grey255 after -s flag processing.",
            )

    def test_strength_parameter_variation(self):
        """Verify that different 'strength' parameter values
        produce different color tables."""
        self._run_colors_enhance(strength=90)
        strength_90 = self._get_color_table("test_red")

        for m in self.rgb_maps:
            self._set_color_table(m, "grey255")

        self._run_colors_enhance(strength=99)
        strength_99 = self._get_color_table("test_red")

        self.assertNotEqual(
            strength_90.strip(),
            strength_99.strip(),
            msg="Different strength values should produce different color tables.",
        )


if __name__ == "__main__":
    test()
