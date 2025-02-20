"""
Name:       r.info test
Purpose:    Tests r.info and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
                License (>=v2). Read the file COPYING that comes with GRASS
                for details.
"""

import json

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestReport(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.json_format_expected_no_stats = {
            "north": 228500,
            "south": 215000,
            "east": 645000,
            "west": 630000,
            "nsres": 10,
            "ewres": 10,
            "rows": 1350,
            "cols": 1500,
            "min": 34300,
            "max": 43600,
            "cells": 2025000,
            "datatype": "CELL",
            "ncats": 43600,
            "map": "lakes",
            "maptype": "raster",
            "mapset": "PERMANENT",
            "date": "Fri Jan 19 23:49:34 2007",
            "creator": "helena",
            "title": "South-West Wake county: Wake county lakes",
            "timestamp": None,
            "units": None,
            "vdatum": None,
            "semantic_label": None,
            "source1": "",
            "source2": "",
            "description": "generated by r.mapcalc",
            "comments": "1 * lakes_large",
        }

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def test_flagg(self):
        """Testing flag g with map geology_30m using simple module"""
        output_str = """north=228500
        south=215000
        east=645000
        west=630000
        nsres=10
        ewres=10
        rows=1350
        cols=1500
        cells=2025000
        datatype=CELL
        ncats=43600"""
        self.assertModuleKeyValue(
            module="r.info",
            map="lakes",
            flags="g",
            reference=output_str,
            precision=2,
            sep="=",
        )

    def test_flagr(self):
        """Testing flag r with map landcover_1m using simple module"""
        output_str = """min=34300
        max=43600"""
        self.assertModuleKeyValue(
            module="r.info",
            map="lakes",
            flags="r",
            reference=output_str,
            precision=2,
            sep="=",
        )

    def test_flage(self):
        """Testing flag e with map lsat7_2002_50"""
        self.assertModule("r.info", map="lakes", flags="e")

    def test_flagh(self):
        """Testing flag h with map zipcodes"""
        self.assertModule("r.info", map="lakes", flags="h")

    def _test_format_json_helper(self, module, expected):
        self.runModule(module)
        result = json.loads(module.outputs.stdout)

        # the following fields vary with the Grass sample data's path
        # therefore only check for their presence in the JSON output
        # and not exact values
        remove_fields = ["location", "project", "database"]
        for field in remove_fields:
            self.assertIn(field, result)
            result.pop(field)

        self.assertCountEqual(list(expected.keys()), list(result.keys()))

        for key, value in expected.items():
            if isinstance(value, float):
                self.assertAlmostEqual(value, result[key])
            else:
                self.assertEqual(value, result[key])

    def test_format_json(self):
        """Testing using simple module in json format"""
        module = SimpleModule("r.info", map="lakes", flags="g", format="json")
        self._test_format_json_helper(module, self.json_format_expected_no_stats)

    def test_sflag_format_json(self):
        """Testing using simple module in json format with sflag"""
        expected_json_with_stats = {
            **self.json_format_expected_no_stats,
            "n": 36011,
            "sum": 1404513600,
            "stddev": 739.7965366431155,
            "mean": 39002.349282163785,
        }
        module = SimpleModule("r.info", map="lakes", flags="gs", format="json")
        self._test_format_json_helper(module, expected_json_with_stats)


class TestComments(TestCase):
    """Check printing of comments"""

    def test_comments_one_line(self):
        """Check that one line is text without any newlines"""
        module = SimpleModule("r.info", map="lakes", format="json")
        self.runModule(module)
        result = json.loads(module.outputs.stdout)
        self.assertFalse(result["comments"].endswith("\n"))
        self.assertEqual(result["comments"], "1 * lakes_large")

    def test_comments_continued_line(self):
        """Check that continued lines are merged"""
        module = SimpleModule("r.info", map="elevation", format="json")
        self.runModule(module)
        result = json.loads(module.outputs.stdout)
        self.assertFalse(result["comments"].endswith("\n"))
        self.assertEqual(
            result["comments"],
            'r.proj input="ned03arcsec" location="northcarolina_latlong" '
            'mapset="helena" output="elev_ned10m" method="cubic" resolution=10',
        )

    def test_comments_multiple_lines(self):
        """Check multiple lines are preserved"""
        module = SimpleModule("r.info", map="lsat7_2002_30", format="json")
        self.runModule(module)
        result = json.loads(module.outputs.stdout)
        self.assertFalse(result["comments"].endswith("\n"))

        lines = result["comments"].splitlines()
        self.assertEqual(
            len(lines),
            31,
        )
        self.assertEqual(
            lines[0],
            'r.in.gdal input="p016r035_7t20020524_z17_nn30_nc_spm_wake.tif" output="lsat7_2002_30"',
        )
        self.assertEqual(
            lines[-1],
            'i.landsat.rgb "b=lsat7_2002_10" "g=lsat7_2002_20" "r=lsat7_2002_30"',
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
