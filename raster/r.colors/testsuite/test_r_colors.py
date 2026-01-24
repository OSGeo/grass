"""Test r.colors module

Tests for r.colors color table assignment and manipulation.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import grass.script as gs


class TestRColorsBasic(TestCase):
    """Test basic r.colors functionality"""

    @classmethod
    def setUpClass(cls):
        """Set up test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()

    def test_color_table_applied(self):
        """Test that color table is actually applied"""
        self.assertModule("r.colors", map="elevation", color="viridis")
        # Verify color table was applied by checking output
        rules = gs.read_command("r.colors.out", map="elevation")
        self.assertIn("nv", rules, "Color table should contain null value rule")
        self.assertGreater(
            len(rules.split("\n")), 5, "Color table should have multiple rules"
        )

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
        # First apply a color table
        self.assertModule("r.colors", map="elevation", color="viridis")

        # Remove it
        self.assertModule("r.colors", map="elevation", flags="r")

        # Check it was removed (should have default grey)
        rules = gs.read_command("r.colors.out", map="elevation")
        # After removal, should have minimal rules
        self.assertLess(
            len(rules.split("\n")), 10, "Removed color table should have minimal rules"
        )

    def test_invert_colors(self):
        """Test inverting colors with -n flag"""
        self.assertModule("r.colors", map="elevation", color="viridis")
        rules_normal = gs.read_command("r.colors.out", map="elevation")

        self.assertModule("r.colors", map="elevation", color="viridis", flags="n")
        rules_inverted = gs.read_command("r.colors.out", map="elevation")

        self.assertNotEqual(
            rules_normal, rules_inverted, "Inverted colors should differ from normal"
        )


class TestRColorsRules(TestCase):
    """Test r.colors with custom rules"""

    @classmethod
    def setUpClass(cls):
        """Set up test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()

    def test_custom_rules_stdin(self):
        """Test applying custom color rules via stdin"""
        rules = "0 blue\n100 green\n200 red"
        self.assertModule("r.colors", map="elevation", rules="-", stdin_=rules)

        # Verify custom rules were applied
        output = gs.read_command("r.colors.out", map="elevation")
        self.assertIn("blue", output.lower(), "Should contain blue color")
        self.assertIn("green", output.lower(), "Should contain green color")
        self.assertIn("red", output.lower(), "Should contain red color")


class TestRColorsColorTables(TestCase):
    """Test standard color tables"""

    @classmethod
    def setUpClass(cls):
        """Set up test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()

    def test_color_viridis(self):
        """Test viridis color table"""
        self.assertModule("r.colors", map="elevation", color="viridis")

    def test_color_grey(self):
        """Test grey color table"""
        self.assertModule("r.colors", map="elevation", color="grey")

    def test_color_rainbow(self):
        """Test rainbow color table"""
        self.assertModule("r.colors", map="elevation", color="rainbow")

    def test_color_elevation(self):
        """Test elevation color table"""
        self.assertModule("r.colors", map="elevation", color="elevation")

    def test_color_slope(self):
        """Test slope color table"""
        self.assertModule("r.colors", map="elevation", color="slope")


if __name__ == "__main__":
    test()
