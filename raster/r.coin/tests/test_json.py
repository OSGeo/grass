"""
Name:      r_coin_json_test
Purpose:   Tests for r.coin JSON output functionality

Author:    Sumit Chintanwar
Copyright: (C) 2026 by Sumit Chintanwar and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import json
import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRCoinJSON(TestCase):
    """Test r.coin JSON output"""

    map1 = "geology"
    map2 = "soils"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.map1)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def run_json(self, unit: str) -> dict:
        output = gs.read_command(
            "r.coin", flags="j", first=self.map1, second=self.map2, units=unit
        )
        return json.loads(output)

    def test_structure(self):
        data = self.run_json("c")

        assert data["module"] == "r.coin"
        assert data["map1"] == self.map1
        assert data["map2"] == self.map2

        assert "coincidence" in data
        assert isinstance(data["coincidence"], list)
        assert len(data["coincidence"]) > 0

        entry = data["coincidence"][0]
        for field in ["cat1", "cat2", "count", "area", "value"]:
            assert field in entry

    def test_entry_fields(self):
        data = self.run_json("c")
        entry = data["coincidence"][0]

        required = ["cat1", "cat2", "count", "area", "value"]
        for field in required:
            assert field in entry

        assert isinstance(entry["cat1"], (int, float))
        assert isinstance(entry["cat2"], (int, float))
        assert isinstance(entry["count"], (int, float))
        assert isinstance(entry["area"], (int, float))
        assert isinstance(entry["value"], (int, float))

        assert entry["count"] > 0
        assert entry["area"] > 0

    def test_unit_names(self):
        units = {
            "c": "cells",
            "p": "percent",
            "x": "percent_of_column",
            "y": "percent_of_row",
            "a": "acres",
            "h": "hectares",
            "k": "square_kilometers",
            "m": "square_miles",
        }

        for code, name in units.items():
            data = self.run_json(code)
            assert data["unit"]["code"] == code
            assert data["unit"]["name"] == name

    def test_cells_equal_count(self):
        data = self.run_json("c")

        for entry in data["coincidence"][:50]:
            assert entry["value"] == entry["count"]

    def test_area_conversions(self):
        data_c = self.run_json("c")
        areas = {
            (int(e["cat1"]), int(e["cat2"])): e["area"] for e in data_c["coincidence"]
        }

        conversions = [
            ("a", 247.105381467165, 1.0),
            ("h", 100.0, 1.0),
            ("k", 1.0, 0.001),
            ("m", 0.386102158542446, 0.001),
        ]

        for unit, factor, tol in conversions:
            data = self.run_json(unit)

            for entry in data["coincidence"][:20]:
                key = (int(entry["cat1"]), int(entry["cat2"]))
                if key not in areas:
                    continue

                expected = (areas[key] / 1000000.0) * factor
                assert abs(entry["value"] - expected) <= tol

    def test_cross_unit_consistency(self):
        data_k = self.run_json("k")
        data_h = self.run_json("h")
        data_a = self.run_json("a")

        km = {
            (int(e["cat1"]), int(e["cat2"])): e["value"] for e in data_k["coincidence"]
        }
        ha = {
            (int(e["cat1"]), int(e["cat2"])): e["value"] for e in data_h["coincidence"]
        }
        ac = {
            (int(e["cat1"]), int(e["cat2"])): e["value"] for e in data_a["coincidence"]
        }

        keys = list(set(km.keys()) & set(ha.keys()) & set(ac.keys()))[:20]

        for key in keys:
            assert abs(ha[key] - km[key] * 100.0) <= 1.0
            assert abs(ac[key] - km[key] * 247.105381467165) <= 1.0

    def test_same_entry_count(self):
        units = ["c", "p", "x", "y", "a", "h", "k", "m"]
        counts = [len(self.run_json(u)["coincidence"]) for u in units]
        assert len(set(counts)) == 1

    def test_same_categories(self):
        data_c = self.run_json("c")
        data_a = self.run_json("a")

        cats_c = {(int(e["cat1"]), int(e["cat2"])) for e in data_c["coincidence"]}
        cats_a = {(int(e["cat1"]), int(e["cat2"])) for e in data_a["coincidence"]}

        assert cats_c == cats_a

    def test_same_area_values(self):
        data_c = self.run_json("c")
        data_a = self.run_json("a")

        areas_c = {
            (int(e["cat1"]), int(e["cat2"])): e["area"] for e in data_c["coincidence"]
        }
        areas_a = {
            (int(e["cat1"]), int(e["cat2"])): e["area"] for e in data_a["coincidence"]
        }

        for key in list(areas_c.keys())[:20]:
            assert areas_c[key] == areas_a[key]

    def test_known_cells(self):
        data = self.run_json("c")
        values = {
            (int(e["cat1"]), int(e["cat2"])): int(e["count"])
            for e in data["coincidence"]
        }

        key = (270, 46348)
        assert key in values
        assert values[key] == 9387

    def test_known_acres(self):
        data = self.run_json("a")
        values = {
            (int(e["cat1"]), int(e["cat2"])): e["value"] for e in data["coincidence"]
        }

        key = (270, 46348)
        assert key in values
        assert abs(values[key] - 2087.62) <= 1.0

    def test_percent_total(self):
        data = self.run_json("p")
        total = sum(e["value"] for e in data["coincidence"])
        assert abs(total - 100.0) <= 0.01


if __name__ == "__main__":
    test()
