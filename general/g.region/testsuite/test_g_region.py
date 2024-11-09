"""Test of g.region

@author Anna Petrasova
"""

import json

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
        line = call_module("g.region", flags="fglecn3", capture_stdout=True)
        self.assertEqual(1, len(line.splitlines()))

    def test_format_json(self):
        """Test json format"""
        expected = {
            "projection": {"code": 99, "name": "Lambert Conformal Conic"},
            "zone": 0,
            "datum": "nad83",
            "ellipsoid": "a=6378137 es=0.006694380022900787",
            "region": {
                "north": 320000,
                "south": 10000,
                "west": 120000,
                "east": 935000,
                "ns-res": 500,
                "ns-res3": 1000,
                "ew-res": 500,
                "ew-res3": 1000,
            },
            "top": 500,
            "bottom": -500,
            "tbres": 100,
            "rows": 620,
            "rows3": 310,
            "cols": 1630,
            "cols3": 815,
            "depths": 10,
            "cells": 1010600,
            "cells3": 2526500,
            "GMT": "120000/935000/10000/320000",
            "WMS": "bbox=120000,10000,935000,320000",
            "se_lat": 33.78822598716895,
            "se_long": -75.48643633119754,
            "sw_lat": 33.722662075471355,
            "sw_long": -84.28378827453474,
            "ew_extent": 815000,
            "ll_clat": 35.17852919352316,
            "ll_clon": -79.91588285974797,
            "ll_e": -75.36388301356145,
            "ll_n": 36.634396311574974,
            "ll_s": 33.722662075471355,
            "ll_w": -84.46788270593447,
            "ne_lat": 36.58069555564894,
            "ne_long": -75.36388301356145,
            "ns_extent": 310000,
            "nw_lat": 36.51287343603797,
            "nw_long": -84.46788270593447,
            "center_easting": 527500,
            "center_lat": 35.23406270825775,
            "center_long": -79.90206638014922,
            "center_northing": 165000,
            "converge_angle": -0.5206458828734528,
        }

        output = call_module("g.region", flags="plectwmn3b", format="json")
        output_json = json.loads(output)

        expected_ellps = expected.pop("ellipsoid").split(" ")
        received_ellps = output_json.pop("ellipsoid").split(" ")
        self.assertEqual(expected_ellps[0], received_ellps[0])
        self.assertAlmostEqual(
            float(expected_ellps[1][3:]), float(received_ellps[1][3:]), places=6
        )
        self.assertCountEqual(list(expected.keys()), list(output_json.keys()))
        for key, value in expected.items():
            if isinstance(value, float):
                self.assertAlmostEqual(value, output_json[key], places=6)
            else:
                self.assertEqual(value, output_json[key])


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
