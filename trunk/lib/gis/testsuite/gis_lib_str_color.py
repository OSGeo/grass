"""Test of gis library string to color conversions

@author Vaclav Petras
"""

from ctypes import byref, c_int

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.lib.gis as libgis


class StringToColorTestCase(TestCase):
    """Test C function G_str_to_color() from gis library"""

    def convert_color(self, string, red, green, blue):
        """General test function to convert string to color

        red, green, blue are expected values.
        """
        r = c_int()
        g = c_int()
        b = c_int()
        ret = libgis.G_str_to_color(string, byref(r), byref(g), byref(b))
        colors = ("{string} -> "
                  "{r.value}, {g.value}, {b.value}".format(**locals()))
        self.assertEqual(ret, 1,
                         msg="Not successful return code (%s)" % colors)
        self.assertEqual(r.value, red,
                         msg="Wrong number for red (%s)" % colors)
        self.assertEqual(g.value, green,
                         msg="Wrong number for green (%s)" % colors)
        self.assertEqual(b.value, blue,
                         msg="Wrong number for blue (%s)" % colors)

    def test_grass_format(self):
        """Test GRASS GIS color format (RRR:GGG:BBB)"""
        self.convert_color("50:150:250", 50, 150, 250)

    def test_grass_format_black(self):
        """Test GRASS GIS color black color"""
        self.convert_color("0:0:0", 0, 0, 0)

    def test_grass_format_white(self):
        """Test GRASS GIS color white color"""
        self.convert_color("255:255:255", 255, 255, 255)

    def test_grass_format_separators(self):
        """Test GRASS GIS color format with all allowed separators"""
        self.convert_color("50,150,250", 50, 150, 250)
        self.convert_color("50:150:250", 50, 150, 250)
        self.convert_color("50;150;250", 50, 150, 250)
        self.convert_color("50 150 250", 50, 150, 250)

    def test_grass_format_multiple_separators(self):
        """Test GRASS GIS color format with duplicated separators"""
        self.convert_color("50, 150, 250", 50, 150, 250)
        self.convert_color("50::150:250", 50, 150, 250)
        self.convert_color("50  ; 150 ; 250", 50, 150, 250)
        self.convert_color("50   150  250", 50, 150, 250)

    def test_grass_format_whitespace(self):
        """Test with whitespace (spaces) around the string"""
        self.convert_color("  50:150:250    ", 50, 150, 250)

    def test_html_hash_hex(self):
        """Test HTML format with hash and hexadecimal (6 letters, #RRGGBB)"""
        self.convert_color("#3296FA", 50, 150, 250)

    def test_html_hash_hex_more_colors(self):
        """Test HTML format with more colors"""
        self.convert_color("#A6CEE3", 166, 206, 227)
        self.convert_color("#33A02C", 51, 160, 44)
        self.convert_color("#FB9A99", 251, 154, 153)
        self.convert_color("#FF7F00", 255, 127, 0)

    def test_html_hash_hex_more_colors_lowercase(self):
        """Test HTML format with lowercase letters"""
        self.convert_color("#a6cee3", 166, 206, 227)
        self.convert_color("#33a02c", 51, 160, 44)
        self.convert_color("#fb9a99", 251, 154, 153)
        self.convert_color("#ff7f00", 255, 127, 0)

    def test_html_hash_hex_more_colors_mixedcase(self):
        """Test HTML format with mixed case letters"""
        self.convert_color("#a6CeE3", 166, 206, 227)
        self.convert_color("#33a02C", 51, 160, 44)
        self.convert_color("#fB9A99", 251, 154, 153)
        self.convert_color("#Ff7f00", 255, 127, 0)

    def test_html_hash_hex_black(self):
        """Test HTML format black color"""
        self.convert_color("#000000", 0, 0, 0)

    def test_html_hash_hex_white(self):
        """Test HTML format white color"""
        self.convert_color("#FFFFFF", 255, 255, 255)

    def test_grass_named_black(self):
        """Test GRASS GIS color black color"""
        self.convert_color("black", 0, 0, 0)

    def test_grass_named_white(self):
        """Test GRASS GIS color white color"""
        self.convert_color("white", 255, 255, 255)


if __name__ == '__main__':
    test()
