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
from datetime import datetime

from grass.gunittest.case import TestCase

from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.utils import xfail_windows


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

    def _assert_report_equal(self, reference, data):
        keys = ["project", "region", "mask", "maps", "totals"]
        for key in keys:
            self.assertEqual(reference[key], data[key])

        for category1, category2 in zip_longest(
            reference["categories"], data["categories"]
        ):
            self.assertEqual(category1["category"], category2["category"])
            self.assertEqual(category1["label"], category2["label"])

            for unit1, unit2 in zip_longest(category1["units"], category2["units"]):
                self.assertEqual(unit1["unit"], unit2["unit"])
                self.assertAlmostEqual(unit1["value"], unit2["value"], places=6)

            for sub_category1, sub_category2 in zip_longest(
                category1["categories"], category2["categories"]
            ):
                self.assertEqual(sub_category1["category"], sub_category2["category"])
                self.assertEqual(sub_category1["label"], sub_category2["label"])

                for sub_unit1, sub_unit2 in zip_longest(
                    sub_category1["units"], sub_category2["units"]
                ):
                    self.assertEqual(sub_unit1["unit"], sub_unit2["unit"])
                    self.assertAlmostEqual(
                        sub_unit1["value"], sub_unit2["value"], places=6
                    )

    def test_json(self):
        """Test JSON format"""
        reference = {
            "project": "nc_spm_full_v2alpha2",
            "region": {
                "north": 228500,
                "south": 215000,
                "east": 645000,
                "west": 630000,
                "ewres": 10,
                "nsres": 10,
            },
            "mask": None,
            "maps": [
                {
                    "name": "towns",
                    "title": "South West Wake: Cities and towns derived from zipcodes",
                    "type": "raster",
                },
                {
                    "name": "zipcodes",
                    "title": "South West Wake: Zipcode areas derived from vector map",
                    "type": "raster",
                },
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "CARY",
                    "units": [
                        {"unit": "cells", "value": 260849},
                        {"unit": "percent", "value": 12.881432098765432},
                    ],
                    "categories": [
                        {
                            "category": 27511,
                            "label": "CARY",
                            "units": [
                                {"unit": "cells", "value": 105800},
                                {"unit": "percent", "value": 40.559864135956055},
                            ],
                        },
                        {
                            "category": 27513,
                            "label": "CARY",
                            "units": [
                                {"unit": "cells", "value": 20530},
                                {"unit": "percent", "value": 7.8704537874402432},
                            ],
                        },
                        {
                            "category": 27518,
                            "label": "CARY",
                            "units": [
                                {"unit": "cells", "value": 134519},
                                {"unit": "percent", "value": 51.569682076603705},
                            ],
                        },
                    ],
                },
                {
                    "category": 2,
                    "label": "GARNER",
                    "units": [
                        {"unit": "cells", "value": 141572},
                        {"unit": "percent", "value": 6.99120987654321},
                    ],
                    "categories": [
                        {
                            "category": 27529,
                            "label": "GARNER",
                            "units": [
                                {"unit": "cells", "value": 141572},
                                {"unit": "percent", "value": 100},
                            ],
                        }
                    ],
                },
                {
                    "category": 3,
                    "label": "APEX",
                    "units": [
                        {"unit": "cells", "value": 25444},
                        {"unit": "percent", "value": 1.2564938271604937},
                    ],
                    "categories": [
                        {
                            "category": 27539,
                            "label": "APEX",
                            "units": [
                                {"unit": "cells", "value": 25444},
                                {"unit": "percent", "value": 100},
                            ],
                        }
                    ],
                },
                {
                    "category": 4,
                    "label": "RALEIGH-CITY",
                    "units": [
                        {"unit": "cells", "value": 160514},
                        {"unit": "percent", "value": 7.926617283950617},
                    ],
                    "categories": [
                        {
                            "category": 27601,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 45468},
                                {"unit": "percent", "value": 28.326501115167524},
                            ],
                        },
                        {
                            "category": 27604,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 47389},
                                {"unit": "percent", "value": 29.523281458315161},
                            ],
                        },
                        {
                            "category": 27605,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 23677},
                                {"unit": "percent", "value": 14.750738253361078},
                            ],
                        },
                        {
                            "category": 27608,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 43980},
                                {"unit": "percent", "value": 27.399479173156237},
                            ],
                        },
                    ],
                },
                {
                    "category": 5,
                    "label": "RALEIGH-SOUTH",
                    "units": [
                        {"unit": "cells", "value": 1227632},
                        {"unit": "percent", "value": 60.623802469135804},
                    ],
                    "categories": [
                        {
                            "category": 27603,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 429179},
                                {"unit": "percent", "value": 34.959906551800536},
                            ],
                        },
                        {
                            "category": 27606,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 662642},
                                {"unit": "percent", "value": 53.977250511553954},
                            ],
                        },
                        {
                            "category": 27610,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 135811},
                                {"unit": "percent", "value": 11.062842936645509},
                            ],
                        },
                    ],
                },
                {
                    "category": 6,
                    "label": "RALEIGH-WEST",
                    "units": [
                        {"unit": "cells", "value": 208989},
                        {"unit": "percent", "value": 10.320444444444444},
                    ],
                    "categories": [
                        {
                            "category": 27607,
                            "label": "RALEIGH",
                            "units": [
                                {"unit": "cells", "value": 208989},
                                {"unit": "percent", "value": 100},
                            ],
                        }
                    ],
                },
            ],
            "totals": [
                {"unit": "cells", "value": 2025000},
                {"unit": "percent", "value": 100},
            ],
        }
        module = SimpleModule("r.report", map="towns,zipcodes", format="json")
        self.runModule(module)
        data = json.loads(module.outputs.stdout)
        self._assert_report_equal(reference, data)

    @xfail_windows
    def test_json2(self):
        """Test JSON format with more options"""
        reference = {
            "project": "nc_spm_full_v2alpha2",
            "created": "2024-07-24T14:59:09+0530",
            "region": {
                "north": 228500,
                "south": 215000,
                "east": 645000,
                "west": 630000,
                "ewres": 10,
                "nsres": 10,
            },
            "mask": None,
            "maps": [
                {
                    "name": "towns",
                    "title": "South West Wake: Cities and towns derived from zipcodes",
                    "type": "raster",
                },
                {
                    "name": "elevation",
                    "title": "South-West Wake county: Elevation NED 10m",
                    "type": "raster",
                },
            ],
            "categories": [
                {
                    "category": 1,
                    "label": "CARY",
                    "units": [
                        {"unit": "square miles", "value": 10.07143619536385},
                        {"unit": "square meters", "value": 26084900},
                        {"unit": "square kilometers", "value": 26.084899999999998},
                        {"unit": "acres", "value": 6445.719165032852},
                        {"unit": "hectares", "value": 2608.4900000000002},
                        {"unit": "cells", "value": 260849},
                        {"unit": "percent", "value": 12.881432098765432},
                    ],
                    "categories": [
                        {
                            "category": 1,
                            "label": "from to",
                            "range": {
                                "from": 55.578792572021484,
                                "to": 105.9543285369873,
                            },
                            "units": [
                                {"unit": "square miles", "value": 0.9655642780829489},
                                {"unit": "square meters", "value": 2500800},
                                {"unit": "square kilometers", "value": 2.5008},
                                {"unit": "acres", "value": 617.9611379730862},
                                {"unit": "hectares", "value": 250.08},
                                {"unit": "cells", "value": 25008},
                                {"unit": "percent", "value": 9.58715578744791},
                            ],
                        },
                        {
                            "category": 2,
                            "label": "from to",
                            "range": {
                                "from": 105.9543285369873,
                                "to": 156.32986450195312,
                            },
                            "units": [
                                {"unit": "square miles", "value": 9.1058719172809},
                                {"unit": "square meters", "value": 23584100},
                                {"unit": "square kilometers", "value": 23.5841},
                                {"unit": "acres", "value": 5827.758027059766},
                                {"unit": "hectares", "value": 2358.4100000000003},
                                {"unit": "cells", "value": 235841},
                                {"unit": "percent", "value": 90.41284421255209},
                            ],
                        },
                    ],
                },
                {
                    "category": 2,
                    "label": "GARNER",
                    "units": [
                        {"unit": "square miles", "value": 5.4661254789171165},
                        {"unit": "square meters", "value": 14157200},
                        {"unit": "square kilometers", "value": 14.1572},
                        {"unit": "acres", "value": 3498.3203065069483},
                        {"unit": "hectares", "value": 1415.72},
                        {"unit": "cells", "value": 141572},
                        {"unit": "percent", "value": 6.99120987654321},
                    ],
                    "categories": [
                        {
                            "category": 1,
                            "label": "from to",
                            "range": {
                                "from": 55.578792572021484,
                                "to": 105.9543285369873,
                            },
                            "units": [
                                {"unit": "square miles", "value": 4.24917008540718},
                                {"unit": "square meters", "value": 11005300},
                                {"unit": "square kilometers", "value": 11.0053},
                                {"unit": "acres", "value": 2719.4688546605908},
                                {"unit": "hectares", "value": 1100.53},
                                {"unit": "cells", "value": 110053},
                                {"unit": "percent", "value": 77.73641680558302},
                            ],
                        },
                        {
                            "category": 2,
                            "label": "from to",
                            "range": {
                                "from": 105.9543285369873,
                                "to": 156.32986450195312,
                            },
                            "units": [
                                {"unit": "square miles", "value": 1.2169553935099355},
                                {"unit": "square meters", "value": 3151900},
                                {"unit": "square kilometers", "value": 3.1519},
                                {"unit": "acres", "value": 778.8514518463573},
                                {"unit": "hectares", "value": 315.19},
                                {"unit": "cells", "value": 31519},
                                {"unit": "percent", "value": 22.263583194416974},
                            ],
                        },
                    ],
                },
                {
                    "category": 3,
                    "label": "APEX",
                    "units": [
                        {"unit": "square miles", "value": 0.9823983321953995},
                        {"unit": "square meters", "value": 2544400},
                        {"unit": "square kilometers", "value": 2.5444},
                        {"unit": "acres", "value": 628.7349326050546},
                        {"unit": "hectares", "value": 254.44000000000003},
                        {"unit": "cells", "value": 25444},
                        {"unit": "percent", "value": 1.2564938271604937},
                    ],
                    "categories": [
                        {
                            "category": 1,
                            "label": "from to",
                            "range": {
                                "from": 55.578792572021484,
                                "to": 105.9543285369873,
                            },
                            "units": [
                                {"unit": "square miles", "value": 0.03262563239683668},
                                {"unit": "square meters", "value": 84500},
                                {
                                    "unit": "square kilometers",
                                    "value": 0.08449999999999999,
                                },
                                {"unit": "acres", "value": 20.880404733975443},
                                {"unit": "hectares", "value": 8.450000000000001},
                                {"unit": "cells", "value": 845},
                                {"unit": "percent", "value": 3.321018707750354},
                            ],
                        },
                        {
                            "category": 2,
                            "label": "from to",
                            "range": {
                                "from": 105.9543285369873,
                                "to": 156.32986450195312,
                            },
                            "units": [
                                {"unit": "square miles", "value": 0.9497726997985628},
                                {"unit": "square meters", "value": 2459900},
                                {
                                    "unit": "square kilometers",
                                    "value": 2.4598999999999998,
                                },
                                {"unit": "acres", "value": 607.8545278710792},
                                {"unit": "hectares", "value": 245.99},
                                {"unit": "cells", "value": 24599},
                                {"unit": "percent", "value": 96.67898129224965},
                            ],
                        },
                    ],
                },
                {
                    "category": 4,
                    "label": "RALEIGH-CITY",
                    "units": [
                        {"unit": "square miles", "value": 6.1974801876282175},
                        {"unit": "square meters", "value": 16051400},
                        {"unit": "square kilometers", "value": 16.0514},
                        {"unit": "acres", "value": 3966.387320082052},
                        {"unit": "hectares", "value": 1605.14},
                        {"unit": "cells", "value": 160514},
                        {"unit": "percent", "value": 7.926617283950617},
                    ],
                    "categories": [
                        {
                            "category": 1,
                            "label": "from to",
                            "range": {
                                "from": 55.578792572021484,
                                "to": 105.9543285369873,
                            },
                            "units": [
                                {"unit": "square miles", "value": 5.062455672160989},
                                {"unit": "square meters", "value": 13111700},
                                {
                                    "unit": "square kilometers",
                                    "value": 13.111699999999999,
                                },
                                {"unit": "acres", "value": 3239.971630183027},
                                {"unit": "hectares", "value": 1311.17},
                                {"unit": "cells", "value": 131117},
                                {"unit": "percent", "value": 81.68570965772456},
                            ],
                        },
                        {
                            "category": 2,
                            "label": "from to",
                            "range": {
                                "from": 105.9543285369873,
                                "to": 156.32986450195312,
                            },
                            "units": [
                                {"unit": "square miles", "value": 1.1350245154672285},
                                {"unit": "square meters", "value": 2939700},
                                {
                                    "unit": "square kilometers",
                                    "value": 2.9396999999999998,
                                },
                                {"unit": "acres", "value": 726.415689899025},
                                {"unit": "hectares", "value": 293.97},
                                {"unit": "cells", "value": 29397},
                                {"unit": "percent", "value": 18.31429034227544},
                            ],
                        },
                    ],
                },
                {
                    "category": 5,
                    "label": "RALEIGH-SOUTH",
                    "units": [
                        {"unit": "square miles", "value": 47.39913650957801},
                        {"unit": "square meters", "value": 122763200},
                        {"unit": "square kilometers", "value": 122.7632},
                        {"unit": "acres", "value": 30335.44736612987},
                        {"unit": "hectares", "value": 12276.32},
                        {"unit": "cells", "value": 1227632},
                        {"unit": "percent", "value": 60.6238024691358},
                    ],
                    "categories": [
                        {
                            "category": 1,
                            "label": "from to",
                            "range": {
                                "from": 55.578792572021484,
                                "to": 105.9543285369873,
                            },
                            "units": [
                                {"unit": "square miles", "value": 24.823086925931666},
                                {"unit": "square meters", "value": 64291500},
                                {"unit": "square kilometers", "value": 64.2915},
                                {"unit": "acres", "value": 15886.775632596238},
                                {"unit": "hectares", "value": 6429.150000000001},
                                {"unit": "cells", "value": 642915},
                                {"unit": "percent", "value": 52.37033573579053},
                            ],
                        },
                        {
                            "category": 2,
                            "label": "from to",
                            "range": {
                                "from": 105.9543285369873,
                                "to": 156.32986450195312,
                            },
                            "units": [
                                {"unit": "square miles", "value": 22.576049583646338},
                                {"unit": "square meters", "value": 58471700},
                                {"unit": "square kilometers", "value": 58.4717},
                                {"unit": "acres", "value": 14448.671733533633},
                                {"unit": "hectares", "value": 5847.17},
                                {"unit": "cells", "value": 584717},
                                {"unit": "percent", "value": 47.62966426420947},
                            ],
                        },
                    ],
                },
                {
                    "category": 6,
                    "label": "RALEIGH-WEST",
                    "units": [
                        {"unit": "square miles", "value": 8.069110401162725},
                        {"unit": "square meters", "value": 20898900},
                        {"unit": "square kilometers", "value": 20.898899999999998},
                        {"unit": "acres", "value": 5164.230656744135},
                        {"unit": "hectares", "value": 2089.8900000000003},
                        {"unit": "cells", "value": 208989},
                        {"unit": "percent", "value": 10.320444444444444},
                    ],
                    "categories": [
                        {
                            "category": 1,
                            "label": "from to",
                            "range": {
                                "from": 55.578792572021484,
                                "to": 105.9543285369873,
                            },
                            "units": [
                                {"unit": "square miles", "value": 0.23822503182068916},
                                {"unit": "square meters", "value": 617000},
                                {"unit": "square kilometers", "value": 0.617},
                                {"unit": "acres", "value": 152.4640203652408},
                                {"unit": "hectares", "value": 61.7},
                                {"unit": "cells", "value": 6170},
                                {"unit": "percent", "value": 2.9523084947054627},
                            ],
                        },
                        {
                            "category": 2,
                            "label": "from to",
                            "range": {
                                "from": 105.9543285369873,
                                "to": 156.32986450195312,
                            },
                            "units": [
                                {"unit": "square miles", "value": 7.8308853693420355},
                                {"unit": "square meters", "value": 20281900},
                                {"unit": "square kilometers", "value": 20.2819},
                                {"unit": "acres", "value": 5011.766636378894},
                                {"unit": "hectares", "value": 2028.19},
                                {"unit": "cells", "value": 202819},
                                {"unit": "percent", "value": 97.04769150529454},
                            ],
                        },
                    ],
                },
            ],
            "totals": [
                {"unit": "square miles", "value": 78.18568710484531},
                {"unit": "square meters", "value": 202500000},
                {"unit": "square kilometers", "value": 202.5},
                {"unit": "acres", "value": 50038.839747100916},
                {"unit": "hectares", "value": 20250},
                {"unit": "cells", "value": 2025000},
                {"unit": "percent", "value": 100},
            ],
        }
        module = SimpleModule(
            "r.report",
            map="towns,elevation",
            units=[
                "miles",
                "meters",
                "kilometers",
                "acres",
                "hectares",
                "cells",
                "percent",
            ],
            nsteps=2,
            format="json",
        )
        self.runModule(module)
        data = json.loads(module.outputs.stdout)

        # created field represents the time of running the command. Therefore, its exact value
        # cannot be tested. We only check that it is present and in the ISO8601 datetime format
        self.assertIn("created", data)
        try:
            # on Python 3.11 and below, datetime.fromisoformat doesn't support zone info with offset
            datetime.strptime(data["created"], "%Y-%m-%dT%H:%M:%S%z")
        except ValueError:
            self.fail("created field is not in isoformat: %s" % (data["created"],))

        self._assert_report_equal(reference, data)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
