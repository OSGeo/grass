"""g.proj tests

(C) 2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:author: Anna Petrasova
"""

import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class GProjTestCase(TestCase):
    """Test cases for g.proj module."""

    # Expected projection keys for different outputs
    # note that proj lib definition is used for all
    # coordinate operations if available

    # since PROJ6, projection info is no longer consistent
    # between GRASS native info and info derived directly from
    # PROJ lib using SRID or WKT because GRASS native info is
    # incomplete with regard to datum

    # reference for output generated with proj lib: wkt, proj4
    reference_proj = [
        "proj",
        "ellps",
        "lat_1",
        "lat_2",
        "lat_0",
        "lon_0",
        "x_0",
        "y_0",
        "no_defs",
    ]

    # reference for output generated from old GRASS projinfo: shell, json
    reference_grass = [
        "proj",
        "a",
        "lat_1",
        "lat_2",
        "lat_0",
        "lon_0",
        "x_0",
        "y_0",
        "no_defs",
    ]

    def assert_keys_in_proj_output(self, output, prefix=""):
        """Helper method to assert that each key (optionally prefixed) exists in the given output string."""
        for key in self.reference_proj:
            self.assertIn(prefix + key, output)

    def assert_keys_in_grass_output(self, output, prefix=""):
        """Helper method to assert that each key (optionally prefixed) exists in the given output string."""
        for key in self.reference_grass:
            self.assertIn(prefix + key, output)

    def test_wkt_output(self):
        """Test if g.proj returns WKT2, WKT1 is not accepted"""
        module_flag = SimpleModule("g.proj", flags="p", format="wkt")
        self.assertModule(module_flag)
        result_flag = module_flag.outputs.stdout
        self.assertIn("PROJCRS", result_flag)

        module_format = SimpleModule("g.proj", flags="p", format="wkt")
        self.assertModule(module_format)
        result_format = module_format.outputs.stdout
        self.assertIn("PROJCRS", result_format)

        self.assertEqual(result_flag, result_format)

    def test_proj_info_output(self):
        """Test if g.proj returns consistent projection info."""
        module = SimpleModule("g.proj", flags="p")
        self.assertModule(module)
        result = module.outputs.stdout
        self.assertIn("PROJ_INFO", result)
        self.assert_keys_in_grass_output(result)

    def test_proj4_output(self):
        """Test if g.proj returns consistent PROJ4 output."""
        module_flag = SimpleModule("g.proj", flags="p", format="proj4")
        self.assertModule(module_flag)
        result_flag = module_flag.outputs.stdout
        self.assert_keys_in_proj_output(result_flag, "+")

        module_format = SimpleModule("g.proj", flags="p", format="proj4")
        self.assertModule(module_format)
        result_format = module_format.outputs.stdout
        self.assert_keys_in_proj_output(result_format, "+")

        self.assertEqual(result_flag, result_format)

    def test_shell_output(self):
        """Test if g.proj returns consistent shell output."""
        module_flag = SimpleModule("g.proj", flags="p", format="shell")
        self.assertModule(module_flag)
        result_flag = module_flag.outputs.stdout
        self.assert_keys_in_grass_output(result_flag)

        module_format = SimpleModule("g.proj", flags="p", format="shell")
        self.assertModule(module_format)
        result_format = module_format.outputs.stdout
        self.assert_keys_in_grass_output(result_format)

        self.assertEqual(result_flag, result_format)

    def test_projjson_output(self):
        """Test if g.proj returns consistent projection info in JSON format."""
        # proj has its own PROJJSON format, use this?
        module = SimpleModule("g.proj", flags="p", format="projjson")
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        self.assertEqual("ProjectedCRS", result["type"])
        # the base GEOGCRS must be "NAD83(HARN)" with corresponding EPSG code
        self.assertEqual("EPSG", result["base_crs"]["id"]["authority"])
        self.assertEqual(4152, result["base_crs"]["id"]["code"])


if __name__ == "__main__":
    test()
