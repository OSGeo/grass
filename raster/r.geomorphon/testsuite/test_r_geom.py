"""
Name:       r.geomorphon tests
Purpose:    Tests r.geomorphon input parsing.
            Uses NC Basic data set.

Author:     Luca Delucchi, Markus Neteler, Sumit Chintanwar
Copyright:  (C) 2017 by Luca Delucchi, Markus Neteler and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command
import grass.script as gs
import unittest
import os
import json

synth_out = """1	flat
3	ridge
4	shoulder
6	slope
8	footslope
9	valley
"""
ele_out = """1	flat
2	peak
3	ridge
4	shoulder
5	spur
6	slope
7	hollow
8	footslope
9	valley
10	pit
"""


@unittest.skipIf(os.getenv("CI") == "true", "Skipping slow tests in CI")
class TestClipling(TestCase):
    inele = "elevation"
    insint = "synthetic_dem"
    outele = "ele_geomorph"
    outsint = "synth_geomorph"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)
        cls.runModule(
            "r.mapcalc",
            expression="{ou} = sin(x() / 5.0) + (sin(x() / 5.0) * 100.0 + 200)".format(
                ou=cls.insint
            ),
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(cls.insint, cls.outele, cls.outsint),
        )
        cls.del_temp_region()

    def test_ele(self):
        self.runModule(
            "r.geomorphon", elevation=self.inele, forms=self.outele, search=10
        )
        category = read_command("r.category", map=self.outele)
        self.assertEqual(first=ele_out, second=category)

        stats = gs.parse_command("r.univar", flags="g", map=self.outele)
        self.assertGreater(
            float(stats["stddev"]),
            1.0,
            "Map has too little variation, algorithm might be failing to classify.",
        )
        mean_val = float(stats["mean"])
        self.assertTrue(
            5.0 < mean_val < 8.0, msg="Mean landform category is out of expected range"
        )

    def test_sint(self):
        """Test r.geomorphon with synthetic data"""
        self.runModule(
            "r.geomorphon", elevation=self.insint, forms=self.outsint, search=10
        )
        category = read_command("r.category", map=self.outsint)
        self.assertEqual(first=synth_out, second=category)

        info = gs.raster_info(self.insint)
        x = info["west"] + 10
        # Ensure vertical consistency in geomorphon results
        values = []
        for frac in [0.1, 0.3, 0.5, 0.7, 0.9]:
            y = info["south"] + frac * (info["north"] - info["south"])
            val = (
                read_command(
                    "r.what",
                    map=self.outsint,
                    coordinates=f"{x},{y}",
                )
                .strip()
                .split("|")[-1]
            )
            values.append(val)

        self.assertTrue(
            all(v == values[0] for v in values),
            "Geomorphon output varies along Y where terrain is invariant",
        )


class TestParameterValidation(TestCase):
    """Test critical parameter validation"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_skip_less_than_search(self):
        """Test that skip radius must be less than search radius"""
        self.assertModuleFail(
            "r.geomorphon", elevation=self.inele, forms="test_out", search=5, skip=5
        )

    def test_flatness_positive(self):
        """Test that flatness threshold must be positive"""
        self.assertModuleFail(
            "r.geomorphon", elevation=self.inele, forms="test_out", search=10, flat=0
        )

    def test_no_output_fails(self):
        """Test that at least one output is required"""
        self.assertModuleFail("r.geomorphon", elevation=self.inele, search=10)


class TestMultipleOutputs(TestCase):
    """Test different output types"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        """Remove test outputs"""
        outputs = ["test_ternary", "test_intensity", "test_elongation"]
        existing = [o for o in outputs if gs.find_file(name=o, element="cell")["file"]]
        if existing:
            self.runModule("g.remove", flags="f", type="raster", name=existing)

    def test_multiple_outputs_combined(self):
        """Test multiple outputs in a single r.geomorphon call"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            ternary="test_ternary",
            intensity="test_intensity",
            elongation="test_elongation",
            search=10,
        )
        self.assertRasterExists("test_ternary")
        self.assertRasterExists("test_intensity")
        self.assertRasterExists("test_elongation")

        stats = gs.parse_command("r.univar", flags="g", map="test_ternary")
        self.assertGreaterEqual(float(stats["min"]), 0)
        self.assertLessEqual(float(stats["max"]), 6560)

        info = gs.raster_info("test_intensity")
        self.assertEqual(info["datatype"], "FCELL")

        stats_intensity = gs.parse_command("r.univar", flags="g", map="test_intensity")
        self.assertGreater(float(stats_intensity["stddev"]), 0.0)

        info_elongation = gs.raster_info("test_elongation")
        self.assertEqual(info_elongation["datatype"], "FCELL")

        stats_elongation = gs.parse_command(
            "r.univar", flags="g", map="test_elongation"
        )
        self.assertGreaterEqual(float(stats_elongation["min"]), 0)
        self.assertLess(float(stats_elongation["max"]), 100)


@unittest.skipIf(os.getenv("CI") == "true", "Skipping slow tests in CI")
class TestFlags(TestCase):
    """Test flags"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        outputs = [
            "test_extended",
            "test_meters",
            "test_basic",
            "test_cells",
            "test_diff",
        ]
        existing = [o for o in outputs if gs.find_file(name=o, element="cell")["file"]]
        if existing:
            self.runModule("g.remove", flags="f", type="raster", name=existing)

    def test_extended_flag(self):
        """Test extended form correction flag"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_basic",
            search=20,
            overwrite=True,
        )
        self.assertModule(
            "r.geomorphon",
            flags="e",
            elevation=self.inele,
            forms="test_extended",
            search=20,
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_basic != test_extended, 1, null())",
            overwrite=True,
        )

        stats_diff = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats_diff["n"]), 0)

    def test_meter_units_flag(self):
        """Test using meters instead of cells for search units"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_cells",
            search=30,
            overwrite=True,
        )
        self.assertModule(
            "r.geomorphon",
            flags="m",
            elevation=self.inele,
            forms="test_meters",
            search=30,
            overwrite=True,
        )
        category_meters = read_command("r.category", map="test_meters")
        self.assertIn("flat", category_meters)

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_cells != test_meters, 1, null())",
            overwrite=True,
        )
        stats_diff = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats_diff["n"]), 0)


@unittest.skipIf(os.getenv("CI") == "true", "Skipping slow tests in CI")
class TestComparisonModes(TestCase):
    """Test different comparison modes for zenith/nadir line-of-sight"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        outputs = ["test_anglev1", "test_anglev2", "test_anglev2_dist"]
        existing = [o for o in outputs if gs.find_file(name=o, element="cell")["file"]]
        if existing:
            self.runModule("g.remove", flags="f", type="raster", name=existing)

    def _run_and_validate(self, name, comparison):
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms=name,
            search=10,
            comparison=comparison,
        )

        category = read_command("r.category", map=name)
        self.assertIn("flat", category)

    def test_anglev1_mode(self):
        """Test anglev1 comparison mode (default)"""
        self._run_and_validate("test_anglev1", "anglev1")

    def test_anglev2_mode(self):
        self._run_and_validate("test_anglev1", "anglev1")
        self._run_and_validate("test_anglev2", "anglev2")

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_anglev1 != test_anglev2, 1, null())",
            overwrite=True,
        )

        stats = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats["n"]), 0)

    def test_anglev2_distance_mode(self):
        self._run_and_validate("test_anglev1", "anglev1")
        self._run_and_validate("test_anglev2_dist", "anglev2_distance")

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_anglev1 != test_anglev2_dist, 1, null())",
            overwrite=True,
        )

        stats = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats["n"]), 0)


class TestProfileFormat(TestCase):
    """Test profile output format parameter"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)

        info = gs.raster_info(cls.inele)
        cls.test_easting = (info["east"] + info["west"]) / 2
        cls.test_northing = (info["north"] + info["south"]) / 2

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_json_profile_format(self):
        """Test JSON profile format output"""
        profile_file = gs.tempfile()
        gs.try_remove(profile_file)

        try:
            self.assertModule(
                "r.geomorphon",
                elevation=self.inele,
                search=10,
                coordinates=(self.test_easting, self.test_northing),
                profiledata=profile_file,
                profileformat="json",
            )

            with open(profile_file) as f:
                data = json.load(f)

            self.assertIn("computation_parameters", data)
            self.assertIn("intermediate_data", data)
            self.assertIn("final_results", data)

            comp = data["computation_parameters"]
            self.assertIn("search_cells", comp)
            self.assertIn("flat_thresh_deg", comp)

            final = data["final_results"]
            for key in [
                "landform_cat",
                "landform_code",
                "landform_name",
                "azimuth",
                "elongation",
                "intensity_m",
            ]:
                self.assertIn(key, final)

            self.assertIn("format_version_major", data)
            self.assertIn("format_version_minor", data)

        finally:
            gs.try_remove(profile_file)


if __name__ == "__main__":
    test()
