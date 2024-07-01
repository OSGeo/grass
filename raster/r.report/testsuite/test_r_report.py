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

    def test_flage(self):
        """Testing flag 'e' with map elevation"""
        self.assertModule("r.report", map="elevation", flags="e")

    def test_flagc(self):
        """Testing flag 'c' with map elevation"""
        self.assertModule("r.report", map="elevation", flags="c")

    def test_flagf(self):
        """Testing flag 'f' with map lakes"""
        self.assertModule("r.report", map="lakes", flags="f")

    def test_flagh(self):
        """Testing flag 'h' with map lakes"""
        self.assertModule("r.report", map="lakes", flags="h")

    def test_flagn(self):
        """Testing flag 'n' with map elevation"""
        self.assertModule("r.report", map="elevation", flags="n")

    def test_flaga(self):
        """Testing flag 'a' with map lakes"""
        self.assertModule("r.report", map="lakes", flags="a")

    def test_output(self):
        """Checking file existence"""
        self.assertModule("r.report", map="lakes", output=self.outfile)
        self.assertFileExists(self.outfile)

    def test_json(self):
        """Test JSON format"""
        reference = {
            "location": "nc_spm_08_grass7",
            "region": {
                "north": 228500,
                "south": 215000,
                "east": 645000,
                "west": 630000,
                "ew_res": 10,
                "ns_res": 10,
            },
            "mask": "none",
            "maps": [
                {
                    "name": "towns",
                    "description": "PERMANENT",
                    "layer": "towns",
                    "type": "raster",
                },
                {
                    "name": "zipcodes",
                    "description": "PERMANENT",
                    "layer": "zipcodes",
                    "type": "raster",
                },
            ],
            "categories": [
                {
                    "category": 1,
                    "description": "CARY",
                    "units": [
                        {"unit": "cell counts", "value": 260849},
                        {"unit": "% cover", "value": 12.881432098765432},
                    ],
                    "categories": [
                        {
                            "category": 27511,
                            "description": "CARY",
                            "units": [
                                {"unit": "cell counts", "value": 105800},
                                {"unit": "% cover", "value": 40.559864135956055},
                            ],
                        },
                        {
                            "category": 27513,
                            "description": "CARY",
                            "units": [
                                {"unit": "cell counts", "value": 20530},
                                {"unit": "% cover", "value": 7.8704537874402432},
                            ],
                        },
                        {
                            "category": 27518,
                            "description": "CARY",
                            "units": [
                                {"unit": "cell counts", "value": 134519},
                                {"unit": "% cover", "value": 51.569682076603705},
                            ],
                        },
                    ],
                },
                {
                    "category": 2,
                    "description": "GARNER",
                    "units": [
                        {"unit": "cell counts", "value": 141572},
                        {"unit": "% cover", "value": 6.99120987654321},
                    ],
                    "categories": [
                        {
                            "category": 27529,
                            "description": "GARNER",
                            "units": [
                                {"unit": "cell counts", "value": 141572},
                                {"unit": "% cover", "value": 100},
                            ],
                        }
                    ],
                },
                {
                    "category": 3,
                    "description": "APEX",
                    "units": [
                        {"unit": "cell counts", "value": 25444},
                        {"unit": "% cover", "value": 1.2564938271604937},
                    ],
                    "categories": [
                        {
                            "category": 27539,
                            "description": "APEX",
                            "units": [
                                {"unit": "cell counts", "value": 25444},
                                {"unit": "% cover", "value": 100},
                            ],
                        }
                    ],
                },
                {
                    "category": 4,
                    "description": "RALEIGH-CITY",
                    "units": [
                        {"unit": "cell counts", "value": 160514},
                        {"unit": "% cover", "value": 7.926617283950617},
                    ],
                    "categories": [
                        {
                            "category": 27601,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 45468},
                                {"unit": "% cover", "value": 28.326501115167524},
                            ],
                        },
                        {
                            "category": 27604,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 47389},
                                {"unit": "% cover", "value": 29.523281458315161},
                            ],
                        },
                        {
                            "category": 27605,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 23677},
                                {"unit": "% cover", "value": 14.750738253361078},
                            ],
                        },
                        {
                            "category": 27608,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 43980},
                                {"unit": "% cover", "value": 27.399479173156237},
                            ],
                        },
                    ],
                },
                {
                    "category": 5,
                    "description": "RALEIGH-SOUTH",
                    "units": [
                        {"unit": "cell counts", "value": 1227632},
                        {"unit": "% cover", "value": 60.623802469135804},
                    ],
                    "categories": [
                        {
                            "category": 27603,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 429179},
                                {"unit": "% cover", "value": 34.959906551800536},
                            ],
                        },
                        {
                            "category": 27606,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 662642},
                                {"unit": "% cover", "value": 53.977250511553954},
                            ],
                        },
                        {
                            "category": 27610,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 135811},
                                {"unit": "% cover", "value": 11.062842936645509},
                            ],
                        },
                    ],
                },
                {
                    "category": 6,
                    "description": "RALEIGH-WEST",
                    "units": [
                        {"unit": "cell counts", "value": 208989},
                        {"unit": "% cover", "value": 10.320444444444444},
                    ],
                    "categories": [
                        {
                            "category": 27607,
                            "description": "RALEIGH",
                            "units": [
                                {"unit": "cell counts", "value": 208989},
                                {"unit": "% cover", "value": 100},
                            ],
                        }
                    ],
                },
            ],
            "totals": [
                {"unit": "cell counts", "value": 2025000},
                {"unit": "% cover", "value": 100},
            ],
        }

        module = SimpleModule("r.report", map="towns,zipcodes", format="json")
        self.runModule(module)
        data = json.loads(module.outputs.stdout)

        keys = ["location", "region", "mask", "maps", "totals"]
        for key in keys:
            self.assertEqual(reference[key], data[key])

        for category1, category2 in zip_longest(
            reference["categories"], data["categories"]
        ):
            self.assertEqual(category1["category"], category2["category"])
            self.assertEqual(category1["description"], category2["description"])

            for unit1, unit2 in zip_longest(category1["units"], category2["units"]):
                self.assertEqual(unit1["unit"], unit2["unit"])
                self.assertAlmostEqual(unit1["value"], unit2["value"], places=6)

            for sub_category1, sub_category2 in zip_longest(
                category1["categories"], category2["categories"]
            ):
                self.assertEqual(sub_category1["category"], sub_category2["category"])
                self.assertEqual(
                    sub_category1["description"], sub_category2["description"]
                )

                for sub_unit1, sub_unit2 in zip_longest(
                    sub_category1["units"], sub_category2["units"]
                ):
                    self.assertEqual(sub_unit1["unit"], sub_unit2["unit"])
                    self.assertAlmostEqual(
                        sub_unit1["value"], sub_unit2["value"], places=6
                    )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
