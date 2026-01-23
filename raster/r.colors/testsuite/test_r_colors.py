"""Test r.colors module"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module


class TestRColors(TestCase):
    """Test r.colors module"""

    @classmethod
    def setUpClass(cls):
        """Set up test environment"""
        cls.use_temp_region()
        call_module("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()

    def test_module_help(self):
        """Test that module help works"""
        self.assertModule("r.colors", help=True)

    def test_color_grey(self):
        """Test applying grey color table"""
        self.assertModule("r.colors", map="elevation", color="grey")

    def test_color_viridis(self):
        """Test applying viridis color table"""
        self.assertModule("r.colors", map="elevation", color="viridis")

    def test_color_rainbow(self):
        """Test applying rainbow color table"""
        self.assertModule("r.colors", map="elevation", color="rainbow")

    def test_color_aspect(self):
        """Test applying aspect color table"""
        self.assertModule("r.colors", map="elevation", color="aspect")

    def test_color_bcyr(self):
        """Test applying bcyr color table"""
        self.assertModule("r.colors", map="elevation", color="bcyr")

    def test_color_bgyr(self):
        """Test applying bgyr color table"""
        self.assertModule("r.colors", map="elevation", color="bgyr")

    def test_color_blues(self):
        """Test applying blues color table"""
        self.assertModule("r.colors", map="elevation", color="blues")

    def test_color_celsius(self):
        """Test applying celsius color table"""
        self.assertModule("r.colors", map="elevation", color="celsius")

    def test_color_corine(self):
        """Test applying corine color table"""
        self.assertModule("r.colors", map="elevation", color="corine")

    def test_color_curvature(self):
        """Test applying curvature color table"""
        self.assertModule("r.colors", map="elevation", color="curvature")

    def test_color_differences(self):
        """Test applying differences color table"""
        self.assertModule("r.colors", map="elevation", color="differences")

    def test_color_elevation(self):
        """Test applying elevation color table"""
        self.assertModule("r.colors", map="elevation", color="elevation")

    def test_color_etopo2(self):
        """Test applying etopo2 color table"""
        self.assertModule("r.colors", map="elevation", color="etopo2")

    def test_color_evi(self):
        """Test applying evi color table"""
        self.assertModule("r.colors", map="elevation", color="evi")

    def test_color_fahrenheit(self):
        """Test applying fahrenheit color table"""
        self.assertModule("r.colors", map="elevation", color="fahrenheit")

    def test_color_gdd(self):
        """Test applying gdd color table"""
        self.assertModule("r.colors", map="elevation", color="gdd")

    def test_color_grass(self):
        """Test applying grass color table"""
        self.assertModule("r.colors", map="elevation", color="grass")

    def test_color_greens(self):
        """Test applying greens color table"""
        self.assertModule("r.colors", map="elevation", color="greens")

    def test_color_gyr(self):
        """Test applying gyr color table"""
        self.assertModule("r.colors", map="elevation", color="gyr")

    def test_color_haxby(self):
        """Test applying haxby color table"""
        self.assertModule("r.colors", map="elevation", color="haxby")

    def test_color_kelvin(self):
        """Test applying kelvin color table"""
        self.assertModule("r.colors", map="elevation", color="kelvin")

    def test_color_ndvi(self):
        """Test applying ndvi color table"""
        self.assertModule("r.colors", map="elevation", color="ndvi")

    def test_color_ndwi(self):
        """Test applying ndwi color table"""
        self.assertModule("r.colors", map="elevation", color="ndwi")

    def test_color_oranges(self):
        """Test applying oranges color table"""
        self.assertModule("r.colors", map="elevation", color="oranges")

    def test_color_population(self):
        """Test applying population color table"""
        self.assertModule("r.colors", map="elevation", color="population")

    def test_color_precipitation(self):
        """Test applying precipitation color table"""
        self.assertModule("r.colors", map="elevation", color="precipitation")

    def test_color_rainbow(self):
        """Test applying rainbow color table"""
        self.assertModule("r.colors", map="elevation", color="rainbow")

    def test_color_ramp(self):
        """Test applying ramp color table"""
        self.assertModule("r.colors", map="elevation", color="ramp")

    def test_color_random(self):
        """Test applying random color table"""
        self.assertModule("r.colors", map="elevation", color="random")

    def test_color_reds(self):
        """Test applying reds color table"""
        self.assertModule("r.colors", map="elevation", color="reds")

    def test_color_roygbiv(self):
        """Test applying roygbiv color table"""
        self.assertModule("r.colors", map="elevation", color="roygbiv")

    def test_color_ryb(self):
        """Test applying ryb color table"""
        self.assertModule("r.colors", map="elevation", color="ryb")

    def test_color_ryg(self):
        """Test applying ryg color table"""
        self.assertModule("r.colors", map="elevation", color="ryg")

    def test_color_sepia(self):
        """Test applying sepia color table"""
        self.assertModule("r.colors", map="elevation", color="sepia")

    def test_color_slope(self):
        """Test applying slope color table"""
        self.assertModule("r.colors", map="elevation", color="slope")

    def test_color_srtm(self):
        """Test applying srtm color table"""
        self.assertModule("r.colors", map="elevation", color="srtm")

    def test_color_terrain(self):
        """Test applying terrain color table"""
        self.assertModule("r.colors", map="elevation", color="terrain")

    def test_color_wave(self):
        """Test applying wave color table"""
        self.assertModule("r.colors", map="elevation", color="wave")


if __name__ == "__main__":
    test()
