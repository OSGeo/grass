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
        # Get rules in JSON format
        output = gs.parse_command("r.colors.out", map="elevation", format="json")
        self.assertIsNotNone(output, "Color table should be returned")

        rules = output["table"]
        self.assertTrue(len(rules) > 0, "Color table should not be empty")

        first_rule = rules[0]
        last_rule = rules[-1]

        self.assertEqual(first_rule["color"], "#000000", "First rule should be black")
        self.assertEqual(last_rule["color"], "#FFFFFF", "Last rule should be white")

    def test_equalize_flag(self):
        """Test equalize histogram flag"""
        self.assertModule("r.colors", map="elevation", color="grey", flags="e")
        rules_eq = gs.read_command("r.colors.out", map="elevation")

        self.assertModule("r.colors", map="elevation", color="grey")
        rules_normal = gs.read_command("r.colors.out", map="elevation")

        self.assertNotEqual(
            rules_eq, rules_normal, "Equalized colors should differ from normal"
        )

    def test_remove_color_table(self):
        """Test removing color table with -r flag"""
        self.assertModule("r.colors", map="elevation", color="grey")
        rules_grey = gs.read_command("r.colors.out", map="elevation")

        self.assertModule("r.colors", map="elevation", flags="r")
        rules_after = gs.read_command("r.colors.out", map="elevation")

        self.assertNotEqual(
            rules_grey, rules_after, "Removed color should revert to default"
        )

    def test_logarithmic_scaling(self):
        """Test logarithmic scaling flag"""
        self.assertModule("r.colors", map="elevation", color="viridis", flags="g")
        rules = gs.read_command("r.colors.out", map="elevation")
        self.assertGreater(len(rules), 0, "Should have color rules")


class TestRColorsRules(TestCase):
    """Test r.colors with custom rules"""

    def test_custom_rules_stdin(self):
        """Test applying custom color rules via stdin"""
        rules = "0 blue\n100 green\n200 red"
        self.assertModule("r.colors", map="elevation", rules="-", stdin_=rules)

        output = gs.read_command("r.colors.out", map="elevation")
        self.assertIn("0:0:255", output, "Should contain blue RGB")
        self.assertIn("0:255:0", output, "Should contain green RGB")
        self.assertIn("255:0:0", output, "Should contain red RGB")

    def test_percentage_rules(self):
        """Test percentage-based color rules"""
        rules = "0% blue\n50% yellow\n100% red"
        self.assertModule("r.colors", map="elevation", rules="-", stdin_=rules)

        output = gs.read_command("r.colors.out", map="elevation")
        self.assertIn("0:0:255", output, "Should contain blue RGB at 0%")
        self.assertIn("255:255:0", output, "Should contain yellow RGB at 50%")
        self.assertIn("255:0:0", output, "Should contain red RGB at 100%")


class TestRColorsOptions(TestCase):
    """Test r.colors options"""

    def test_raster_option(self):
        """Test using another raster's color table"""
        # Set distinct color table for source
        self.assertModule("r.colors", map="elevation", color="viridis")

        # Create target raster
        self.runModule("r.mapcalc", expression="test_raster = elevation * 2")
        self.addCleanup(
            self.runModule, "g.remove", flags="f", type="raster", name="test_raster"
        )

        # Copy colors
        self.assertModule("r.colors", map="test_raster", raster="elevation")

        # Verify color tables match exactly
        output_src = gs.parse_command("r.colors.out", map="elevation", format="json")
        output_dst = gs.parse_command("r.colors.out", map="test_raster", format="json")

        rules_src = output_src["table"]
        rules_dst = output_dst["table"]

        self.assertTrue(len(rules_dst) > 0, "Copied color table should not be empty")
        self.assertEqual(
            len(rules_src), len(rules_dst), "Color table length should match"
        )

        # Verify all hex colors match (ignoring 'val' as data ranges differ)
        for i in range(len(rules_src)):
            src = rules_src[i]
            dst = rules_dst[i]
            self.assertEqual(src["color"], dst["color"], f"Rule {i} Color mismatch")

        # Check null value and default colors
        self.assertEqual(
            output_src.get("nv"), output_dst.get("nv"), "Null Value color mismatch"
        )
        self.assertEqual(
            output_src.get("default"),
            output_dst.get("default"),
            "Default color mismatch",
        )


if __name__ == "__main__":
    test()
