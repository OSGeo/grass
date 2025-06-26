"""Test of g.region

@author Anna Petrasova
"""

import json
from copy import deepcopy

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import call_module
import grass.script as gs


class TestRegion(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def test_d_flag(self):
        n = 228500
        res = 1
        n_default = 320000
        res_default = 500

        self.runModule("g.region", res=res, n=n)
        region = gs.region()
        self.assertEqual(n, region["n"])
        self.assertEqual(res, region["nsres"])

        # test default with no update flag
        self.runModule("g.region", flags="dup")
        region = gs.region()
        self.assertEqual(n, region["n"])
        self.assertEqual(res, region["nsres"])

        # test set default
        self.runModule("g.region", flags="d")
        region = gs.region()
        self.assertEqual(n_default, region["n"])
        self.assertEqual(res_default, region["nsres"])

    def test_f_flag(self):
        line = call_module("g.region", flags="flecn3", capture_stdout=True)
        self.assertEqual(1, len(line.splitlines()))

        # Test that the -f flag ignores the format option
        plain_result = call_module(
            "g.region", flags="flecn3", format="plain", capture_stdout=True
        )
        self.assertEqual(plain_result, line)

        shell_result = call_module(
            "g.region", flags="flecn3", format="shell", capture_stdout=True
        )
        self.assertEqual(shell_result, line)

        json_result = call_module(
            "g.region", flags="flecn3", format="json", capture_stdout=True
        )
        self.assertEqual(json_result, line)

    def test_plain_format(self):
        """Test plain format with plectwmn3b flags"""
        actual = call_module(
            "g.region", flags="plectwmn3b", capture_stdout=True
        ).splitlines()
        expected = [
            "projection:         99 (Lambert Conformal Conic)",
            "zone:               0",
            "datum:              nad83",
            "ellipsoid:          a=6378137 es=0.006694380022900787",
            "north:              320000",
            "south:              10000",
            "west:               120000",
            "east:               935000",
            "top:                500.00000000",
            "bottom:             -500.00000000",
            "nsres:              500",
            "nsres3:             1000",
            "ewres:              500",
            "ewres3:             1000",
            "tbres:              100",
            "rows:               620",
            "rows3:              310",
            "cols:               1630",
            "cols3:              815",
            "depths:             10",
            "cells:              1010600",
            "cells3:             2526500",
            "north-west corner:  long: 84:28:04.377741W lat: 36:30:46.34437N",
            "north-east corner:  long: 75:21:49.978849W lat: 36:34:50.504N",
            "south-east corner:  long: 75:29:11.170792W lat: 33:47:17.613554N",
            "south-west corner:  long: 84:17:01.637788W lat: 33:43:21.583472N",
            "center longitude:   79:54:07.438969W",
            "center latitude:    35:14:02.62575N",
            "north-south extent: 310000.000000",
            "east-west extent:   815000.000000",
            "center easting:     527500.000000",
            "center northing:    165000.000000",
            "120000/935000/10000/320000",
            "bbox=120000,10000,935000,320000",
            "convergence angle:  -0.520646",
            "north latitude:      36:38:03.826722N",
            "south latitude:      33:43:21.583472N",
            "west longitude:      84:28:04.377741W",
            "east longitude:      75:21:49.978849W",
            "center longitude:   79:54:57.178295W",
            "center latitude:     35:10:42.705097N",
        ]
        self.assertEqual(actual, expected)

        plain_actual = call_module(
            "g.region", flags="plectwmn3b", format="plain", capture_stdout=True
        ).splitlines()
        self.assertEqual(plain_actual, actual)

    def test_shell_format(self):
        """Test shell format with plectwmn3b flags"""
        actual = call_module(
            "g.region", flags="gplectwmn3b", capture_stdout=True
        ).splitlines()
        expected = [
            "projection=99",
            "zone=0",
            "n=320000",
            "s=10000",
            "w=120000",
            "e=935000",
            "t=500",
            "b=-500",
            "nsres=500",
            "nsres3=1000",
            "ewres=500",
            "ewres3=1000",
            "tbres=100",
            "rows=620",
            "rows3=310",
            "cols=1630",
            "cols3=815",
            "depths=10",
            "cells=1010600",
            "cells3=2526500",
            "nw_long=-84.46788271",
            "nw_lat=36.51287344",
            "ne_long=-75.36388301",
            "ne_lat=36.58069556",
            "se_long=-75.48643633",
            "se_lat=33.78822599",
            "sw_long=-84.28378827",
            "sw_lat=33.72266208",
            "center_long=-79.90206638",
            "center_lat=35.23406271",
            "ns_extent=310000.000000",
            "ew_extent=815000.000000",
            "center_easting=527500.000000",
            "center_northing=165000.000000",
            "120000/935000/10000/320000",
            "bbox=120000,10000,935000,320000",
            "converge_angle=-0.520646",
            "ll_n=36.63439631",
            "ll_s=33.72266208",
            "ll_w=-84.46788271",
            "ll_e=-75.36388301",
            "ll_clon=-79.91588286",
            "ll_clat=35.17852919",
        ]
        self.assertEqual(actual, expected)

        shell_actual = call_module(
            "g.region", flags="plectwmn3b", format="shell", capture_stdout=True
        ).splitlines()
        self.assertEqual(shell_actual, actual)

    def test_format_json(self):
        """Test default json format output"""
        expected = {
            "north": 320000,
            "south": 10000,
            "west": 120000,
            "east": 935000,
            "nsres": 500,
            "ewres": 500,
            "rows": 620,
            "cols": 1630,
            "cells": 1010600,
        }

        output = call_module("g.region", flags="p", format="json")
        output_json = json.loads(output)

        self.assertEqual(expected, output_json)

    def test_json_with_flags(self):
        """Test json format with plectwmn3b flags"""
        expected = {
            "north": 320000,
            "south": 10000,
            "west": 120000,
            "east": 935000,
            "nsres": 500,
            "ewres": 500,
            "rows": 620,
            "cols": 1630,
            "nsres3": 1000,
            "ewres3": 1000,
            "top": 500,
            "bottom": -500,
            "rows3": 310,
            "cols3": 815,
            "tbres": 100,
            "depths": 10,
            "cells": 1010600,
            "cells3": 2526500,
            "nw_long": -84.46788270593447,
            "nw_lat": 36.51287343603797,
            "ne_long": -75.36388301356145,
            "ne_lat": 36.58069555564893,
            "se_long": -75.48643633119754,
            "se_lat": 33.788225987168964,
            "sw_long": -84.28378827453474,
            "sw_lat": 33.72266207547134,
            "center_long": -79.90206638014922,
            "center_lat": 35.23406270825776,
            "ns_extent": 310000,
            "ew_extent": 815000,
            "center_easting": 527500,
            "center_northing": 165000,
            "GMT": "120000/935000/10000/320000",
            "WMS": "bbox=120000,10000,935000,320000",
            "converge_angle": -0.5206458828749483,
            "ll_n": 36.634396311574996,
            "ll_s": 33.72266207547134,
            "ll_w": -84.46788270593447,
            "ll_e": -75.36388301356145,
            "ll_clon": -79.91588285974797,
            "ll_clat": 35.17852919352317,
        }

        output = call_module("g.region", flags="plectwmn3b", format="json")
        output_json = json.loads(output)

        self.assertCountEqual(list(expected.keys()), list(output_json.keys()))
        for key, value in expected.items():
            if isinstance(value, float):
                self.assertAlmostEqual(value, output_json[key], places=6)
            else:
                self.assertEqual(value, output_json[key])

    def test_2d_region(self):
        region_before = gs.region()
        test_nsres = region_before["nsres"] / 2
        test_ewres = region_before["ewres"] / 2
        self.runModule("g.region", nsres=test_nsres, ewres=test_ewres)
        region_after = gs.region()
        self.assertAlmostEqual(test_nsres, region_after["nsres"])
        self.assertAlmostEqual(test_ewres, region_after["ewres"])

    def test_3d_region(self):
        # Make sure region is default
        self.runModule("g.region", flags="d")
        region_before = deepcopy(gs.region(region3d=True))
        test_nsres3 = region_before["nsres3"] / 5
        test_ewres3 = region_before["ewres3"] / 2
        test_tbres = region_before["tbres"] / 4
        precision = 7
        self.runModule(
            "g.region", nsres3=test_nsres3, ewres3=test_ewres3, tbres=test_tbres
        )
        region_after = gs.region(region3d=True)
        # Setting 3D resolution does not change the extent or 2D resolution
        self.assertEqual(region_before["n"], region_after["n"])
        self.assertEqual(region_before["s"], region_after["s"])
        self.assertEqual(region_before["e"], region_after["e"])
        self.assertEqual(region_before["w"], region_after["w"])
        self.assertEqual(region_before["t"], region_after["t"])
        self.assertEqual(region_before["b"], region_after["b"])
        self.assertEqual(region_before["nsres"], region_after["nsres"])
        self.assertEqual(region_before["ewres"], region_after["ewres"])

        # 3D resolution settings are applied correctly
        self.assertAlmostEqual(region_after["nsres3"], test_nsres3, places=precision)
        self.assertAlmostEqual(region_after["ewres3"], test_ewres3, places=precision)
        self.assertAlmostEqual(region_after["tbres"], test_tbres, places=precision)

        # res3 is applied to all 3D resolutions
        test_res3 = 2
        self.runModule("g.region", res3=test_res3)
        region_after = gs.region(region3d=True)
        self.assertAlmostEqual(region_after["nsres3"], test_res3, places=precision)
        self.assertAlmostEqual(region_after["ewres3"], test_res3, places=precision)
        self.assertAlmostEqual(region_after["tbres"], test_res3, places=precision)

        # res3 is overridden by individual settings
        # tbres
        self.runModule("g.region", res3=test_res3, tbres=test_tbres)
        region_after = gs.region(region3d=True)
        self.assertAlmostEqual(region_after["nsres3"], test_res3, places=precision)
        self.assertAlmostEqual(region_after["ewres3"], test_res3, places=precision)
        self.assertAlmostEqual(region_after["tbres"], test_tbres, places=precision)
        # ewres3
        self.runModule("g.region", res3=test_res3, ewres3=test_ewres3)
        region_after = gs.region(region3d=True)
        self.assertAlmostEqual(region_after["nsres3"], test_res3, places=precision)
        self.assertAlmostEqual(region_after["ewres3"], test_ewres3, places=precision)
        self.assertAlmostEqual(region_after["tbres"], test_res3, places=precision)
        # nsres3
        self.runModule("g.region", res3=test_res3, nsres3=test_nsres3)
        region_after = gs.region(region3d=True)
        self.assertAlmostEqual(region_after["nsres3"], test_nsres3, places=precision)
        self.assertAlmostEqual(region_after["ewres3"], test_res3, places=precision)
        self.assertAlmostEqual(region_after["tbres"], test_res3, places=precision)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
