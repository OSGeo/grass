"""Test t.info format=json output.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

Validates that t.info format=json returns valid JSON and that
parsed output contains correct metadata values (not only keys).
"""

import json

import grass.script as gs

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestTinfoJson(TestCase):
    """Value-based tests for t.info format=json."""

    @classmethod
    def setUpClass(cls):
        """Set region and create a space time raster dataset with one map."""
        cls.use_temp_region()
        cls.runModule(
            "g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10, flags="p3"
        )
        cls.runModule(
            "r.mapcalc", expression="prec_1 = 1", overwrite=True
        )
        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            type="raster",
            flags="i",
            input="precip_abs1",
            maps="prec_1",
            start="2001-01-01",
            increment="20 years",
            overwrite=True,
        )
        cls.mapset = gs.gisenv()["MAPSET"]

    @classmethod
    def tearDownClass(cls):
        """Remove dataset and temporary region."""
        cls.runModule("t.remove", flags="df", type="strds", inputs="precip_abs1")
        cls.del_temp_region()

    def test_t_info_format_json_returns_valid_json(self):
        """t.info format=json outputs valid JSON (parseable with json.loads)."""
        info = SimpleModule("t.info", format="json", type="strds", input="precip_abs1")
        self.assertModule(info)
        out = info.outputs.stdout
        self.assertIsNotNone(out)
        parsed = json.loads(out)
        self.assertIsInstance(parsed, dict)

    def test_t_info_format_json_metadata_values(self):
        """Parsed JSON from t.info format=json has correct metadata values."""
        info = SimpleModule("t.info", format="json", type="strds", input="precip_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)

        self.assertEqual(parsed["name"], "precip_abs1")
        self.assertEqual(parsed["mapset"], self.mapset)
        self.assertEqual(parsed["id"], f"precip_abs1@{self.mapset}")
        self.assertEqual(parsed["temporal_type"], "absolute")

        self.assertEqual(parsed["north"], 80)
        self.assertEqual(parsed["south"], 0)
        self.assertEqual(parsed["east"], 120)
        self.assertEqual(parsed["west"], 0)

        self.assertEqual(parsed["number_of_maps"], 1)
        self.assertEqual(parsed["title"], "A test")
        self.assertEqual(parsed["description"], "A test")

        self.assertEqual(parsed["start_time"], "2001-01-01T00:00:00")
        self.assertEqual(parsed["end_time"], "2021-01-01T00:00:00")
