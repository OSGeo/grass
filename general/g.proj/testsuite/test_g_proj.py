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
    """Test cases for g.proj."""

    def test_wkt_output(self):
        """Test if g.proj returns WKT"""
        module = SimpleModule("g.proj", flags="w")
        self.assertModule(module)
        self.assertIn("PROJCRS", module.outputs.stdout)

    def test_proj_info_output(self):
        """Test if g.proj returns consistent projection info."""
        module = SimpleModule("g.proj", flags="p")
        self.assertModule(module)
        self.assertIn("PROJ_INFO", module.outputs.stdout)

    def test_proj4_output(self):
        """Test if g.proj returns consistent PROJ4 info."""
        module = SimpleModule("g.proj", flags="j")
        self.assertModule(module)

    def test_wkt_output_json(self):
        """Test if g.proj returns consistent WKT in JSON format."""
        module = SimpleModule("g.proj", flags="w", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)
        self.assertTrue("wkt" in result)

    def test_proj_info_output_json(self):
        """Test if g.proj returns consistent projection info in JSON format."""
        reference = [
            "name",
            "proj",
            "datum",
            "a",
            "es",
            "lat_1",
            "lat_2",
            "lat_0",
            "lon_0",
            "x_0",
            "y_0",
            "no_defs",
        ]

        module = SimpleModule("g.proj", flags="p", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)
        for key in reference:
            self.assertTrue(key in result)

    def test_proj4_output_json(self):
        """Test if g.proj returns consistent PROJ4 info in JSON format."""
        module = SimpleModule("g.proj", flags="j", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)
        self.assertTrue("proj4" in result)


if __name__ == "__main__":
    test()
