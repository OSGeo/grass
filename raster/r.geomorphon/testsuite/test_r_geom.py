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
            overwrite=True,
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
        """Test basic forms output with elevation data against reference"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms=self.outele,
            search=10,
            overwrite=True,
        )

        reference = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.82088828626101,
            "stddev": 1.7495396954895,
        }

        self.assertRasterFitsUnivar(
            self.outele,
            reference=reference,
            precision=0.001,
        )

        category = read_command("r.category", map=self.outele)
        self.assertEqual(first=ele_out, second=category)

    def test_sint(self):
        """Test r.geomorphon with synthetic data against reference"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.insint,
            forms=self.outsint,
            search=10,
            overwrite=True,
        )

        reference = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 9,
            "mean": 5.99331056641298,
            "stddev": 0.624993270568342,
        }

        self.assertRasterFitsUnivar(
            self.outsint,
            reference=reference,
            precision=0.001,
        )

        category = read_command("r.category", map=self.outsint)
        self.assertEqual(first=synth_out, second=category)


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
        outputs = [
            "test_forms_multi",
            "test_ternary_multi",
            "test_intensity_multi",
            "test_elongation_multi",
        ]
        existing = [o for o in outputs if gs.find_file(name=o, element="cell")["file"]]
        if existing:
            self.runModule("g.remove", flags="f", type="raster", name=existing)

    def test_multiple_outputs_combined(self):
        """Test multiple outputs in a single call against reference"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_forms_multi",
            ternary="test_ternary_multi",
            intensity="test_intensity_multi",
            elongation="test_elongation_multi",
            search=15,
            overwrite=True,
        )

        reference_forms = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.81997163379065,
            "stddev": 1.80462751945005,
        }

        self.assertRasterFitsUnivar(
            "test_forms_multi",
            reference=reference_forms,
            precision=0.001,
        )

        reference_ternary = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 0,
            "max": 6560,
            "mean": 461.729427565141,
            "stddev": 1035.85787176981,
        }

        self.assertRasterFitsUnivar(
            "test_ternary_multi",
            reference=reference_ternary,
            precision=0.001,
        )

        reference_intensity = {
            "n": 2019304,
            "null_cells": 5696,
            "min": -14.2301387786865,
            "max": 19.7351875305176,
            "mean": 0.401472390593266,
            "stddev": 2.46468990424033,
        }

        self.assertRasterFitsUnivar(
            "test_intensity_multi",
            reference=reference_intensity,
            precision=0.001,
        )

        reference_elongation = {
            "n": 2017213,
            "null_cells": 7787,
            "min": 0.999999940395355,
            "max": 30,
            "mean": 2.07217229639214,
            "stddev": 1.59757907173383,
        }

        self.assertRasterFitsUnivar(
            "test_elongation_multi",
            reference=reference_elongation,
            precision=0.001,
        )


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
        """Test extended form correction flag against reference"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_basic",
            search=20,
            overwrite=True,
        )

        reference_basic = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.81211595678511,
            "stddev": 1.84913018808328,
        }

        self.assertRasterFitsUnivar(
            "test_basic",
            reference=reference_basic,
            precision=0.001,
        )

        self.assertModule(
            "r.geomorphon",
            flags="e",
            elevation=self.inele,
            forms="test_extended",
            search=20,
            overwrite=True,
        )

        reference_extended = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.8660800949238,
            "stddev": 1.79017370086154,
        }

        self.assertRasterFitsUnivar(
            "test_extended",
            reference=reference_extended,
            precision=0.001,
        )

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_basic != test_extended, 1, null())",
            overwrite=True,
        )

        stats_diff = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats_diff["n"]), 0)

    def test_meter_units_flag(self):
        """Test using meters instead of cells for search units against reference"""
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

        reference_meters = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.63588295769235,
            "stddev": 1.75742791972737,
        }

        self.assertRasterFitsUnivar(
            "test_meters",
            reference=reference_meters,
            precision=0.001,
        )

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
        outputs = ["test_anglev1", "test_anglev2", "test_anglev2_dist", "test_diff"]
        existing = [o for o in outputs if gs.find_file(name=o, element="cell")["file"]]
        if existing:
            self.runModule("g.remove", flags="f", type="raster", name=existing)

    def test_anglev1_mode(self):
        """Test anglev1 comparison mode (default) against reference"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_anglev1",
            search=10,
            comparison="anglev1",
            overwrite=True,
        )

        reference = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.82088828626101,
            "stddev": 1.7495396954895,
        }

        self.assertRasterFitsUnivar(
            "test_anglev1",
            reference=reference,
            precision=0.001,
        )

    def test_anglev2_mode(self):
        """Test anglev2 comparison mode against reference"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_anglev1",
            search=10,
            comparison="anglev1",
            overwrite=True,
        )

        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_anglev2",
            search=10,
            comparison="anglev2",
            overwrite=True,
        )

        reference_anglev2 = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.82424092657668,
            "stddev": 1.75081305759747,
        }

        self.assertRasterFitsUnivar(
            "test_anglev2",
            reference=reference_anglev2,
            precision=0.001,
        )

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_anglev1 != test_anglev2, 1, null())",
            overwrite=True,
        )

        stats = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats["n"]), 0)

    def test_anglev2_distance_mode(self):
        """Test anglev2_distance comparison mode against reference"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_anglev1",
            search=10,
            comparison="anglev1",
            overwrite=True,
        )

        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_anglev2_dist",
            search=10,
            comparison="anglev2_distance",
            overwrite=True,
        )

        reference_anglev2_dist = {
            "n": 2019304,
            "null_cells": 5696,
            "min": 1,
            "max": 10,
            "mean": 5.82424092657668,
            "stddev": 1.75081305759747,
        }

        self.assertRasterFitsUnivar(
            "test_anglev2_dist",
            reference=reference_anglev2_dist,
            precision=0.001,
        )

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
                overwrite=True,
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
