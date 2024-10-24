"""
Name:       r.report test
Purpose:    Tests r.report and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

import json
import os
from itertools import zip_longest

from grass.gunittest.case import TestCase

from grass.gunittest.gmodules import SimpleModule


class TestRasterreport(TestCase):
    outfile = "test_out.csv"

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()
        if os.path.isfile(cls.outfile):
            os.remove(cls.outfile)

    def _assert_stats_equal(self, reference, data):
        for item1, item2 in zip_longest(reference, data):
            self.assertCountEqual(list(item1.keys()), list(item2.keys()))
            for category1, category2 in zip_longest(
                item1["categories"], item2["categories"]
            ):
                if "category" in category1:
                    self.assertEqual(category1["category"], category2["category"])
                if "label" in category1:
                    self.assertEqual(category1["label"], category2["label"])
                if "range" in category1:
                    self.assertAlmostEqual(
                        category1["range"]["low"], category2["range"]["low"], places=6
                    )
                    self.assertAlmostEqual(
                        category1["range"]["high"], category2["range"]["high"], places=6
                    )
            self.assertEqual(item1["count"], item2["count"])

    def test_json(self):
        """Test JSON format"""
        reference = [
            {
                "categories": [
                    {"category": 1, "label": "CARY"},
                    {"category": 27511, "label": "CARY"},
                ],
                "count": 105800,
            },
            {
                "categories": [
                    {"category": 1, "label": "CARY"},
                    {"category": 27513, "label": "CARY"},
                ],
                "count": 20530,
            },
            {
                "categories": [
                    {"category": 1, "label": "CARY"},
                    {"category": 27518, "label": "CARY"},
                ],
                "count": 134519,
            },
            {
                "categories": [
                    {"category": 2, "label": "GARNER"},
                    {"category": 27529, "label": "GARNER"},
                ],
                "count": 141572,
            },
            {
                "categories": [
                    {"category": 3, "label": "APEX"},
                    {"category": 27539, "label": "APEX"},
                ],
                "count": 25444,
            },
            {
                "categories": [
                    {"category": 4, "label": "RALEIGH-CITY"},
                    {"category": 27601, "label": "RALEIGH"},
                ],
                "count": 45468,
            },
            {
                "categories": [
                    {"category": 4, "label": "RALEIGH-CITY"},
                    {"category": 27604, "label": "RALEIGH"},
                ],
                "count": 47389,
            },
            {
                "categories": [
                    {"category": 4, "label": "RALEIGH-CITY"},
                    {"category": 27605, "label": "RALEIGH"},
                ],
                "count": 23677,
            },
            {
                "categories": [
                    {"category": 4, "label": "RALEIGH-CITY"},
                    {"category": 27608, "label": "RALEIGH"},
                ],
                "count": 43980,
            },
            {
                "categories": [
                    {"category": 5, "label": "RALEIGH-SOUTH"},
                    {"category": 27603, "label": "RALEIGH"},
                ],
                "count": 429179,
            },
            {
                "categories": [
                    {"category": 5, "label": "RALEIGH-SOUTH"},
                    {"category": 27606, "label": "RALEIGH"},
                ],
                "count": 662642,
            },
            {
                "categories": [
                    {"category": 5, "label": "RALEIGH-SOUTH"},
                    {"category": 27610, "label": "RALEIGH"},
                ],
                "count": 135811,
            },
            {
                "categories": [
                    {"category": 6, "label": "RALEIGH-WEST"},
                    {"category": 27607, "label": "RALEIGH"},
                ],
                "count": 208989,
            },
        ]
        module = SimpleModule(
            "r.stats", flags="cl", input="towns,zipcodes", format="json"
        )
        self.runModule(module)
        data = json.loads(module.outputs.stdout)
        self.assertEqual(reference, data)

    def test_json2(self):
        """Test JSON format with more options"""
        reference_file = os.path.normpath(
            os.path.join(__file__, "..", "test_json2.json")
        )
        with open(reference_file, "r") as f:
            reference = json.load(f)
        module = SimpleModule(
            "r.stats",
            flags="cl",
            input="towns,elevation",
            format="json",
        )
        self.runModule(module)
        data = json.loads(module.outputs.stdout)
        self._assert_stats_equal(reference, data)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
