"""Test r.colors module

Tests for r.colors color table assignment and manipulation.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import grass.script as gs


class TestRColorsBasic(TestCase):
    """Test basic r.colors functionality"""

    def test_color_table_applied(self):
        """Test that color table is actually applied"""
        self.assertModule("r.colors", map="elevation", color="grey")
        # Verify color table was applied using JSON output
        rules = gs.parse_command("r.colors.out", map="elevation", format="json")
        self.assertIsNotNone(rules, "Color table should be returned")

    def test_equalize_flag(self):
        """Test equalize histogram flag"""
        self.assertModule("r.colors", map="elevation", color="grey", flags="e")
        rules_eq = gs.read_command("r.colors.out", map="elevation")

        # Apply without equalize
        self.assertModule("r.colors", map="elevation", color="grey")
        rules_normal = gs.read_command("r.colors.out", map="elevation")

        # Rules should be different
        self.assertNotEqual(
            rules_eq, rules_normal, "Equalized colors should differ from normal"
        )

    def test_remove_color_table(self):
        """Test removing color table with -r flag"""
        # First apply a non-default color table
        self.assertModule("r.colors", map="elevation", color="grey")
        rules_grey = gs.read_command("r.colors.out", map="elevation")

        # Remove it (should revert to default viridis)
        self.assertModule("r.colors", map="elevation", flags="r")
        rules_after = gs.read_command("r.colors.out", map="elevation")

        # After removal, should be different from grey
        self.assertNotEqual(
            rules_grey, rules_after, "Removed color should revert to default"
        )

    def test_logarithmic_scaling(self):
        """Test logarithmic scaling flag"""
        self.assertModule("r.colors", map="elevation", color="viridis", flags="g")
        # Verify it runs without error
        rules = gs.read_command("r.colors.out", map="elevation")
        self.assertGreater(len(rules), 0, "Should have color rules")

    def test_reverse_colors(self):
        """Test reversing color table"""
        # Apply normal color table
        self.assertModule("r.colors", map="elevation", color="grey")
        rules_normal = gs.read_command("r.colors.out", map="elevation")

        # Apply reversed
        self.assertModule("r.colors", map="elevation", color="grey", flags="n")
        rules_reversed = gs.read_command("r.colors.out", map="elevation")

        # Should be different
        self.assertNotEqual(
            rules_normal, rules_reversed, "Reversed colors should differ"
        )


class TestRColorsRules(TestCase):
    """Test r.colors with custom rules"""

    def test_custom_rules_stdin(self):
        """Test applying custom color rules via stdin"""
        rules = "0 blue\n100 green\n200 red"
        self.assertModule("r.colors", map="elevation", rules="-", stdin_=rules)

        # Verify custom rules were applied (output is RGB values)
        output = gs.read_command("r.colors.out", map="elevation")
        # Check for RGB values: blue=0:0:255, green=0:255:0, red=255:0:0
        self.assertIn("0:0:255", output, "Should contain blue RGB")
        self.assertIn("0:255:0", output, "Should contain green RGB")
        self.assertIn("255:0:0", output, "Should contain red RGB")

    def test_percentage_rules(self):
        """Test percentage-based color rules"""
        rules = "0% blue\n50% yellow\n100% red"
        self.assertModule("r.colors", map="elevation", rules="-", stdin_=rules)

        # Verify rules were applied
        output = gs.read_command("r.colors.out", map="elevation")
        self.assertGreater(len(output), 0, "Should have color rules")


class TestRColorsOptions(TestCase):
    """Test r.colors options"""

    def test_raster_option(self):
        """Test using another raster's color table"""
        # First set a color table on elevation
        self.assertModule("r.colors", map="elevation", color="grey")

        # Create another raster and copy elevation's colors
        self.runModule("r.mapcalc", expression="test_raster = elevation * 2")
        self.assertModule("r.colors", map="test_raster", raster="elevation")

        # Cleanup
        self.runModule("g.remove", flags="f", type="raster", name="test_raster")

    def test_offset_option(self):
        """Test offset parameter"""
        self.assertModule("r.colors", map="elevation", color="viridis", offset=100)
        # Verify it runs without error
        rules = gs.read_command("r.colors.out", map="elevation")
        self.assertGreater(len(rules), 0, "Should have color rules")

    def test_scale_option(self):
        """Test scale parameter"""
        self.assertModule("r.colors", map="elevation", color="viridis", scale=2.0)
        # Verify it runs without error
        rules = gs.read_command("r.colors.out", map="elevation")
        self.assertGreater(len(rules), 0, "Should have color rules")


if __name__ == "__main__":
    test()
