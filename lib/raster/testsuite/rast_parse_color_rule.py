"""Test of raster library string color rule to value and color conversions

@author Vaclav Petras

@copyright 2016 by Vaclav Petras and the GRASS Development Team

@license This program is free software under the 
GNU General Public License (>=v2). 
Read the file COPYING that comes with GRASS
for details
"""

from ctypes import byref, c_int, c_double

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.lib.raster import Rast_parse_color_rule, Rast_parse_color_rule_error


# TODO: add also a test class for values
class ParseSingleColorRuleColorsTestCase(TestCase):
    """Color-focused test of C function Rast_parse_color_rule()"""

    def convert_rule(self, string, value, red, green, blue):
        """General test function to convert string to value and color

        value, red, green, blue are expected values.
        """
        # we are not testing value, but color, so using these values
        # so that value and percentage can be used interchangeably
        min_val = 0.
        max_val = 100.
        r = c_int()
        g = c_int()
        b = c_int()
        v = c_double()  # DCELL
        # not testing any of these three
        norm = c_int()
        nv = c_int()
        default = c_int()
        ret = Rast_parse_color_rule(min_val, max_val, string, 
                                    byref(v), byref(r), byref(g), byref(b),
                                    byref(norm), byref(nv), byref(default))
        colors = ("{string} -> "
                  "{v.value}: {r.value}, {g.value}, {b.value}".format(**locals()))
        error_text = Rast_parse_color_rule_error(ret)
        self.assertEqual(error_text, b"",
                         msg=("Conversion not successful (%s): %s (%s)"
                              % (colors, error_text, ret)))
        self.assertEqual(v.value, value,
                         msg="Wrong number for value (%s)" % colors)
        self.assertEqual(r.value, red,
                         msg="Wrong number for red (%s)" % colors)
        self.assertEqual(g.value, green,
                         msg="Wrong number for green (%s)" % colors)
        self.assertEqual(b.value, blue,
                         msg="Wrong number for blue (%s)" % colors)

    def test_grass_format_separators(self):
        """Test GRASS GIS color format with all allowed separators"""
        self.convert_rule("15% 50,150,250", 15, 50, 150, 250)
        self.convert_rule("15% 50:150:250", 15, 50, 150, 250)
        self.convert_rule("15 50;150;250", 15, 50, 150, 250)
        self.convert_rule("15 50 150 250", 15, 50, 150, 250)

    def test_grass_format_multiple_separators(self):
        """Test GRASS GIS color format with duplicated separators"""
        self.convert_rule("15%  50, 150, 250", 15, 50, 150, 250)
        self.convert_rule("15%   50::150:250", 15, 50, 150, 250)
        self.convert_rule("15   50  ; 150 ; 250", 15, 50, 150, 250)
        self.convert_rule("15    50   150  250", 15, 50, 150, 250)

    def test_grass_format_whitespace(self):
        """Test with whitespace (spaces) around and in the string"""
        self.convert_rule("    15%    50:150:250    ", 15, 50, 150, 250)
        self.convert_rule("    15    50  150   250    ", 15, 50, 150, 250)

    def test_html_hash_hex(self):
        """Test HTML format with hash and hexadecimal (6 letters, #RRGGBB)

        comments (with #) and empty lines are handled in
        Rast_read_color_rule() by simple first-character test
        so we have not much here to test
        """
        self.convert_rule("15% #A6CEE3", 15, 166, 206, 227)
        self.convert_rule("15 #33a02c", 15, 51, 160, 44)
        self.convert_rule("15 #fB9A99", 15, 251, 154, 153)

    def test_grass_named(self):
        """Test GRASS GIS named colors"""
        self.convert_rule("15% black   ", 15, 0, 0, 0)
        self.convert_rule("15    white", 15, 255, 255, 255)
        self.convert_rule("   15 white", 15, 255, 255, 255)


if __name__ == '__main__':
    test()
