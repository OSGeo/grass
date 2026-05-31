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

    grand_total_miles = 78.185687104845513

    known_col_totals = {
        217: 28.014105435637472,
        262: 7.671232126784743,
        270: 26.616840334088188,
        405: 9.795797864380406,
        583: 0.834328154394372,
        720: 0.186255681280876,
        766: 0.273128666952926,
        862: 2.383099742955688,
        910: 1.736069745670255,
        921: 0.483708784221976,
        945: 0.000347491942688,
        946: 0.157066358095067,
        948: 0.033706718440756,
    }

    known_row_totals = {
        18683: 0.503168333012516,
        19401: 0.023976944045486,
        19447: 0.426720105621111,
        19548: 0.001042475828065,
        20149: 0.036834145924949,
    }

    known_matrix_miles = {
        (217, 18683): 0.503168333012516,
        (262, 19401): 0.023976944045486,
        (217, 19447): 0.326989918069598,
        (262, 19447): 0.099730187551514,
        (217, 20149): 0.014247169650216,
    }

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.map1)

        cls._data = {}
        for unit in ("c", "p", "a", "h", "k", "m"):
            output = gs.read_command(
                "r.coin",
                flags="j",
                first=cls.map1,
                second=cls.map2,
                units=unit,
            )
            cls._data[unit] = json.loads(output)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def data(self, unit: str) -> dict:
        """Return cached JSON output for the given unit."""
        return self._data[unit]

    def get_cell(self, data, cat1, cat2):
        """Return matrix value for a given (cat1, cat2) pair."""
        c = data["col_cats"].index(cat1)
        r = data["row_cats"].index(cat2)
        return data["matrix"][r][c]

    def test_top_level_keys(self):
        data = self.data("c")
        required = {
            "project",
            "created",
            "region",
            "mask",
            "maps",
            "unit",
            "row_cats",
            "col_cats",
            "matrix",
            "row_totals",
            "row_totals_without_zero",
            "col_totals",
            "col_totals_without_zero",
            "totals",
        }
        for key in required:
            assert key in data, f"missing top-level key: {key}"

    def test_unit_names(self):
        expected = {
            "c": "cells",
            "p": "percent",
            "a": "acres",
            "h": "hectares",
            "k": "square_kilometers",
            "m": "square_miles",
        }
        for code, name in expected.items():
            assert self.data(code)["unit"] == name, f"unit mismatch for code={code}"

    def test_col_cats(self):
        assert self.data("m")["col_cats"] == [
            217,
            262,
            270,
            405,
            583,
            720,
            766,
            862,
            910,
            921,
            945,
            946,
            948,
        ]

    def test_matrix_shape(self):
        data = self.data("c")
        assert len(data["matrix"]) == 3345
        assert len(data["matrix"][0]) == 13

    def test_row_totals_match_matrix_sums(self):
        data = self.data("k")
        for r, row in enumerate(data["matrix"]):
            assert abs(sum(row) - data["row_totals"][r]) < 1e-6, (
                f"row_total mismatch at row {r}"
            )

    def test_col_totals_match_matrix_sums(self):
        data = self.data("k")
        nrows = len(data["row_cats"])
        for c in range(len(data["col_cats"])):
            col_sum = sum(data["matrix"][r][c] for r in range(nrows))
            assert abs(col_sum - data["col_totals"][c]) < 1e-6, (
                f"col_total mismatch at col {c}"
            )

    def test_grand_total_miles(self):
        assert abs(self.data("m")["totals"]["value"] - self.grand_total_miles) < 0.001

    def test_grand_total_percent_is_100(self):
        assert abs(self.data("p")["totals"]["value"] - 100.0) < 0.01

    def test_known_col_totals(self):
        data = self.data("m")
        for cat1, expected in self.known_col_totals.items():
            c = data["col_cats"].index(cat1)
            assert abs(data["col_totals"][c] - expected) < 0.001, (
                f"col_total mismatch for cat1={cat1}"
            )

    def test_known_row_totals(self):
        data = self.data("m")
        for cat2, expected in self.known_row_totals.items():
            r = data["row_cats"].index(cat2)
            assert abs(data["row_totals"][r] - expected) < 0.001, (
                f"row_total mismatch for cat2={cat2}"
            )

    def test_known_matrix_miles(self):
        data = self.data("m")
        for (cat1, cat2), expected in self.known_matrix_miles.items():
            assert abs(self.get_cell(data, cat1, cat2) - expected) < 0.001, (
                f"matrix mismatch for cat1={cat1}, cat2={cat2}"
            )

    def test_cross_unit_km_to_hectares(self):
        data_k = self.data("k")
        data_h = self.data("h")
        for r in range(len(data_k["row_cats"])):
            for c in range(len(data_k["col_cats"])):
                assert (
                    abs(data_h["matrix"][r][c] - data_k["matrix"][r][c] * 100.0) < 0.001
                ), f"km->ha mismatch at row={r}, col={c}"

    def test_cross_unit_km_to_acres(self):
        data_k = self.data("k")
        data_a = self.data("a")
        for r in range(len(data_k["row_cats"])):
            for c in range(len(data_k["col_cats"])):
                assert (
                    abs(
                        data_a["matrix"][r][c]
                        - data_k["matrix"][r][c] * 247.105381467165
                    )
                    < 0.001
                ), f"km->acres mismatch at row={r}, col={c}"

    def test_cross_unit_km_to_miles(self):
        data_k = self.data("k")
        data_m = self.data("m")
        for r in range(len(data_k["row_cats"])):
            for c in range(len(data_k["col_cats"])):
                assert (
                    abs(
                        data_m["matrix"][r][c]
                        - data_k["matrix"][r][c] * 0.386102158542446
                    )
                    < 0.001
                ), f"km->miles mismatch at row={r}, col={c}"


if __name__ == "__main__":
    test()
