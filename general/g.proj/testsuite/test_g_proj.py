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
    reference = [
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

    def assert_keys_in_output(self, output, prefix=""):
        """Helper method to assert that each key (optionally prefixed) exists in the given output string."""
        for key in self.reference:
            self.assertIn(prefix + key, output)

    def test_wkt_output(self):
        """Test if g.proj returns WKT"""
        module_flag = SimpleModule("g.proj", flags="w")
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
        self.assert_keys_in_output(result)

    def test_proj4_output(self):
        """Test if g.proj returns consistent PROJ4 output."""
        module_flag = SimpleModule("g.proj", flags="j")
        self.assertModule(module_flag)
        result_flag = module_flag.outputs.stdout
        self.assert_keys_in_output(result_flag, "+")

        module_format = SimpleModule("g.proj", flags="p", format="proj4")
        self.assertModule(module_format)
        result_format = module_format.outputs.stdout
        self.assert_keys_in_output(result_format, "+")

        self.assertEqual(result_flag, result_format)

    def test_shell_output(self):
        """Test if g.proj returns consistent shell output."""
        module_flag = SimpleModule("g.proj", flags="g")
        self.assertModule(module_flag)
        result_flag = module_flag.outputs.stdout
        self.assert_keys_in_output(result_flag)

        module_format = SimpleModule("g.proj", flags="p", format="shell")
        self.assertModule(module_format)
        result_format = module_format.outputs.stdout
        self.assert_keys_in_output(result_format)

        self.assertEqual(result_flag, result_format)

    def test_proj_info_output_json(self):
        """Test if g.proj returns consistent projection info in JSON format."""
        module = SimpleModule("g.proj", flags="p", format="json")
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        self.assert_keys_in_output(result)


if __name__ == "__main__":
    test()
