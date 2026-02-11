"""Test t.info format=json output for all space-time dataset types.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

Validates that t.info format=json returns valid JSON and that
parsed output contains correct metadata values (not only keys).
Covers STRDS, STVDS, and STR3DS with shell/JSON parity checks.
"""

import json

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.main import test


class TestTinfoJson(TestCase):
    """Value-based tests for t.info format=json."""

    @classmethod
    def setUpClass(cls):
        """Set region and create a space time raster dataset with maps."""
        cls.use_temp_region()
        cls.runModule(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
            flags="p3",
        )
        cls.runModule("r.mapcalc", expression="prec_1 = 1", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_2 = 2", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_3 = 3", overwrite=True)
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
            maps="prec_1,prec_2,prec_3",
            start="2001-01-01",
            increment="1 months",
            overwrite=True,
        )
        cls.mapset = gs.gisenv()["MAPSET"]

    @classmethod
    def tearDownClass(cls):
        """Remove dataset and temporary region."""
        cls.runModule("t.remove", flags="df", type="strds", inputs="precip_abs1")
        cls.del_temp_region()

    def test_json_format_returns_valid_json(self):
        """t.info format=json outputs valid JSON (parseable with json.loads)."""
        info = SimpleModule("t.info", format="json", type="strds", input="precip_abs1")
        self.assertModule(info)
        out = info.outputs.stdout
        self.assertIsNotNone(out)
        parsed = json.loads(out)
        self.assertIsInstance(parsed, dict)

    def test_json_metadata_values(self):
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

        self.assertEqual(parsed["number_of_maps"], 3)
        self.assertEqual(parsed["title"], "A test")
        self.assertEqual(parsed["description"], "A test")

        self.assertEqual(parsed["start_time"], "2001-01-01T00:00:00")
        self.assertEqual(parsed["end_time"], "2001-04-01T00:00:00")

    def test_json_required_keys_present(self):
        """All required metadata keys are present in JSON output."""
        info = SimpleModule("t.info", format="json", type="strds", input="precip_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)

        required_keys = [
            "id",
            "name",
            "mapset",
            "temporal_type",
            "creation_time",
            "start_time",
            "end_time",
            "number_of_maps",
            "north",
            "south",
            "east",
            "west",
            "title",
            "description",
        ]

        for key in required_keys:
            self.assertIn(
                key,
                parsed,
                f"Required key '{key}' missing from JSON output",
            )

    def test_json_raster_statistics_present(self):
        """STRDS JSON output includes raster-specific statistics."""
        info = SimpleModule("t.info", format="json", type="strds", input="precip_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)

        expected_raster_keys = [
            "nsres_min",
            "nsres_max",
            "ewres_min",
            "ewres_max",
            "min_min",
            "min_max",
            "max_min",
            "max_max",
            "aggregation_type",
        ]

        for key in expected_raster_keys:
            self.assertIn(
                key,
                parsed,
                f"STRDS should include raster statistic '{key}'",
            )

    def test_json_shell_parity(self):
        """JSON output contains all keys present in shell format output."""
        # Get shell format output
        shell_info = SimpleModule(
            "t.info", format="shell", type="strds", input="precip_abs1"
        )
        self.assertModule(shell_info)

        # Parse shell output to get keys
        shell_keys = set()
        for line in shell_info.outputs.stdout.strip().split("\n"):
            if "=" in line:
                key = line.split("=")[0]
                shell_keys.add(key)

        # Get JSON output
        json_info = SimpleModule(
            "t.info", format="json", type="strds", input="precip_abs1"
        )
        self.assertModule(json_info)
        json_data = json.loads(json_info.outputs.stdout)
        json_keys = set(json_data.keys())

        # JSON should have all the keys that shell has
        missing_keys = shell_keys - json_keys
        self.assertEqual(
            len(missing_keys),
            0,
            f"JSON output missing keys from shell output: {missing_keys}",
        )

    def test_json_format_via_parser(self):
        """format=json works through the parser (G_OPT_F_FORMAT integration)."""
        info = SimpleModule("t.info", format="json", type="strds", input="precip_abs1")
        self.assertModule(info)
        out = info.outputs.stdout
        self.assertTrue(len(out) > 0)
        parsed = json.loads(out)
        self.assertIsInstance(parsed, dict)

    def test_shell_format_via_option(self):
        """format=shell works as equivalent to -g flag."""
        flag_info = SimpleModule("t.info", flags="g", type="strds", input="precip_abs1")
        self.assertModule(flag_info)

        option_info = SimpleModule(
            "t.info", format="shell", type="strds", input="precip_abs1"
        )
        self.assertModule(option_info)

        # Both should produce the same output
        self.assertEqual(flag_info.outputs.stdout, option_info.outputs.stdout)

    def test_plain_format_still_works(self):
        """Default plain format output still functions correctly."""
        info = SimpleModule("t.info", format="plain", type="strds", input="precip_abs1")
        self.assertModule(info)
        out = info.outputs.stdout
        self.assertIn("precip_abs1", out)


class TestTinfoJsonStvds(TestCase):
    """Value-based tests for t.info format=json on STVDS."""

    @classmethod
    def setUpClass(cls):
        """Set region and create a space time vector dataset with maps."""
        cls.use_temp_region()
        cls.runModule(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
        )
        cls.runModule(
            "v.random",
            output="vect_1",
            npoints=20,
            seed=1,
            overwrite=True,
        )
        cls.runModule(
            "v.random",
            output="vect_2",
            npoints=20,
            seed=2,
            overwrite=True,
        )
        cls.runModule(
            "v.random",
            output="vect_3",
            npoints=20,
            seed=3,
            overwrite=True,
        )
        cls.runModule(
            "t.create",
            type="stvds",
            temporaltype="absolute",
            output="vect_abs1",
            title="A test STVDS",
            description="Test vector time series",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            type="vector",
            flags="i",
            input="vect_abs1",
            maps="vect_1,vect_2,vect_3",
            start="2001-01-01",
            increment="1 months",
            overwrite=True,
        )
        cls.mapset = gs.gisenv()["MAPSET"]

    @classmethod
    def tearDownClass(cls):
        """Remove dataset and temporary region."""
        cls.runModule("t.remove", flags="df", type="stvds", inputs="vect_abs1")
        cls.del_temp_region()

    def test_json_format_returns_valid_json(self):
        """t.info format=json outputs valid JSON for STVDS."""
        info = SimpleModule("t.info", format="json", type="stvds", input="vect_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)
        self.assertIsInstance(parsed, dict)

    def test_json_metadata_values(self):
        """Parsed JSON has correct metadata values for STVDS."""
        info = SimpleModule("t.info", format="json", type="stvds", input="vect_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)

        self.assertEqual(parsed["name"], "vect_abs1")
        self.assertEqual(parsed["mapset"], self.mapset)
        self.assertEqual(parsed["id"], f"vect_abs1@{self.mapset}")
        self.assertEqual(parsed["temporal_type"], "absolute")
        self.assertEqual(parsed["number_of_maps"], 3)
        self.assertEqual(parsed["title"], "A test STVDS")
        self.assertEqual(parsed["description"], "Test vector time series")
        self.assertEqual(parsed["start_time"], "2001-01-01T00:00:00")
        self.assertEqual(parsed["end_time"], "2001-04-01T00:00:00")

    def test_json_vector_metadata_present(self):
        """STVDS JSON output includes vector-specific metadata."""
        info = SimpleModule("t.info", format="json", type="stvds", input="vect_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)

        expected_vector_keys = [
            "points",
            "lines",
            "boundaries",
            "centroids",
            "areas",
            "primitives",
            "number_of_maps",
        ]

        for key in expected_vector_keys:
            self.assertIn(
                key,
                parsed,
                f"STVDS should include vector metadata '{key}'",
            )

    def test_json_shell_parity(self):
        """JSON output contains all keys from shell format for STVDS."""
        shell_info = SimpleModule(
            "t.info",
            format="shell",
            type="stvds",
            input="vect_abs1",
        )
        self.assertModule(shell_info)

        shell_keys = set()
        for line in shell_info.outputs.stdout.strip().split("\n"):
            if "=" in line:
                shell_keys.add(line.split("=")[0])

        json_info = SimpleModule(
            "t.info",
            format="json",
            type="stvds",
            input="vect_abs1",
        )
        self.assertModule(json_info)
        json_keys = set(json.loads(json_info.outputs.stdout).keys())

        missing_keys = shell_keys - json_keys
        self.assertEqual(
            len(missing_keys),
            0,
            f"JSON output missing keys from shell: {missing_keys}",
        )


class TestTinfoJsonStr3ds(TestCase):
    """Value-based tests for t.info format=json on STR3DS."""

    @classmethod
    def setUpClass(cls):
        """Set region and create a space time 3D raster dataset."""
        cls.use_temp_region()
        cls.runModule(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
        )
        cls.runModule("r3.mapcalc", expression="vol_1 = 100", overwrite=True)
        cls.runModule("r3.mapcalc", expression="vol_2 = 200", overwrite=True)
        cls.runModule("r3.mapcalc", expression="vol_3 = 300", overwrite=True)
        cls.runModule(
            "t.create",
            type="str3ds",
            temporaltype="absolute",
            output="vol_abs1",
            title="A test STR3DS",
            description="Test 3D raster time series",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            type="raster_3d",
            flags="i",
            input="vol_abs1",
            maps="vol_1,vol_2,vol_3",
            start="2001-01-01",
            increment="1 months",
            overwrite=True,
        )
        cls.mapset = gs.gisenv()["MAPSET"]

    @classmethod
    def tearDownClass(cls):
        """Remove dataset and temporary region."""
        cls.runModule("t.remove", flags="df", type="str3ds", inputs="vol_abs1")
        cls.del_temp_region()

    def test_json_format_returns_valid_json(self):
        """t.info format=json outputs valid JSON for STR3DS."""
        info = SimpleModule("t.info", format="json", type="str3ds", input="vol_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)
        self.assertIsInstance(parsed, dict)

    def test_json_metadata_values(self):
        """Parsed JSON has correct metadata values for STR3DS."""
        info = SimpleModule("t.info", format="json", type="str3ds", input="vol_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)

        self.assertEqual(parsed["name"], "vol_abs1")
        self.assertEqual(parsed["mapset"], self.mapset)
        self.assertEqual(parsed["id"], f"vol_abs1@{self.mapset}")
        self.assertEqual(parsed["temporal_type"], "absolute")

        self.assertEqual(parsed["north"], 80)
        self.assertEqual(parsed["south"], 0)
        self.assertEqual(parsed["east"], 120)
        self.assertEqual(parsed["west"], 0)
        self.assertEqual(parsed["top"], 50)
        self.assertEqual(parsed["bottom"], 0)

        self.assertEqual(parsed["number_of_maps"], 3)
        self.assertEqual(parsed["title"], "A test STR3DS")
        self.assertEqual(parsed["description"], "Test 3D raster time series")
        self.assertEqual(parsed["start_time"], "2001-01-01T00:00:00")
        self.assertEqual(parsed["end_time"], "2001-04-01T00:00:00")

    def test_json_raster3d_statistics_present(self):
        """STR3DS JSON output includes 3D raster-specific statistics."""
        info = SimpleModule("t.info", format="json", type="str3ds", input="vol_abs1")
        self.assertModule(info)
        parsed = json.loads(info.outputs.stdout)

        expected_raster3d_keys = [
            "nsres_min",
            "nsres_max",
            "ewres_min",
            "ewres_max",
            "tbres_min",
            "tbres_max",
            "min_min",
            "min_max",
            "max_min",
            "max_max",
            "aggregation_type",
        ]

        for key in expected_raster3d_keys:
            self.assertIn(
                key,
                parsed,
                f"STR3DS should include 3D raster statistic '{key}'",
            )

    def test_json_shell_parity(self):
        """JSON output contains all keys from shell format for STR3DS."""
        shell_info = SimpleModule(
            "t.info",
            format="shell",
            type="str3ds",
            input="vol_abs1",
        )
        self.assertModule(shell_info)

        shell_keys = set()
        for line in shell_info.outputs.stdout.strip().split("\n"):
            if "=" in line:
                shell_keys.add(line.split("=")[0])

        json_info = SimpleModule(
            "t.info",
            format="json",
            type="str3ds",
            input="vol_abs1",
        )
        self.assertModule(json_info)
        json_keys = set(json.loads(json_info.outputs.stdout).keys())

        missing_keys = shell_keys - json_keys
        self.assertEqual(
            len(missing_keys),
            0,
            f"JSON output missing keys from shell: {missing_keys}",
        )


if __name__ == "__main__":
    test()
