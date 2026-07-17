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

import json

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command

# A 400 x 400 cell subregion of the elevation raster which contains all
# ten landforms and keeps the tests fast.
region = {"n": 224000, "s": 220000, "w": 635000, "e": 639000, "res": 10}

# Reference statistics were computed with the nc_spm_08_grass7 dataset.
# The elevation raster in nc_spm_full_v2beta1 (used in CI) classifies a
# small number of cells differently, so precisions are chosen to accept
# both datasets.

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


class TestClipling(TestCase):
    inele = "elevation"
    insint = "synthetic_dem"
    outele = "ele_geomorph"
    outsint = "synt_geomorph"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region"""
        cls.use_temp_region()
        cls.runModule("g.region", **region)
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
            name=(cls.outele, cls.outsint, cls.insint),
        )
        cls.del_temp_region()

    def test_ele(self):
        """Test forms output with elevation data against reference statistics"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms=self.outele,
            search=10,
        )

        reference = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.81124214034999,
            "stddev": 1.77419698170854,
        }

        self.assertRasterFitsUnivar(
            self.outele,
            reference=reference,
            precision=0.002,
        )

        category = read_command("r.category", map=self.outele)
        self.assertEqual(first=ele_out, second=category)

    def test_sint(self):
        """Test forms output with synthetic data against reference statistics"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.insint,
            forms=self.outsint,
            search=10,
        )

        reference = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 9,
            "mean": 5.99981061084316,
            "stddev": 0.600696938302586,
        }

        self.assertRasterFitsUnivar(
            self.outsint,
            reference=reference,
            precision=0.002,
        )

        category = read_command("r.category", map=self.outsint)
        self.assertEqual(first=synth_out, second=category)


class TestParameterValidation(TestCase):
    """Test critical parameter validation"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", **region)

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
        cls.runModule("g.region", **region)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        """Remove test outputs"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(
                "test_forms_multi",
                "test_ternary_multi",
                "test_intensity_multi",
                "test_elongation_multi",
            ),
        )

    def test_multiple_outputs_combined(self):
        """Test multiple outputs in a single call against reference statistics"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_forms_multi",
            ternary="test_ternary_multi",
            intensity="test_intensity_multi",
            elongation="test_elongation_multi",
            search=15,
        )

        reference_forms = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.81710057826823,
            "stddev": 1.821438283467,
        }

        self.assertRasterFitsUnivar(
            "test_forms_multi",
            reference=reference_forms,
            precision=0.002,
        )

        reference_ternary = {
            "n": 158404,
            "null_cells": 1596,
            "min": 0,
            "max": 6560,
            "mean": 465.214786242772,
            "stddev": 1038.84517618805,
        }

        self.assertRasterFitsUnivar(
            "test_ternary_multi",
            reference=reference_ternary,
            precision=5,
        )

        reference_intensity = {
            "n": 158404,
            "null_cells": 1596,
            "min": -12.1759548187256,
            "max": 13.1019992828369,
            "mean": 0.506680904574651,
            "stddev": 2.72539083939135,
        }

        self.assertRasterFitsUnivar(
            "test_intensity_multi",
            reference=reference_intensity,
            precision=0.002,
        )

        # Cell counts and the maximum are omitted because the number of
        # null (not applicable) cells and the single longest form depend
        # on the classification which slightly differs between the
        # dataset versions.
        reference_elongation = {
            "min": 0.999999940395355,
            "mean": 2.09059988401335,
            "stddev": 1.58855170683853,
        }

        self.assertRasterFitsUnivar(
            "test_elongation_multi",
            reference=reference_elongation,
            precision=0.002,
        )


class TestFlags(TestCase):
    """Test flags"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", **region)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        """Remove test outputs"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(
                "test_extended",
                "test_meters",
                "test_basic",
                "test_cells",
                "test_diff",
            ),
        )

    def test_extended_flag(self):
        """Test extended form correction flag against reference statistics"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_basic",
            search=20,
        )

        reference_basic = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.82717608141209,
            "stddev": 1.85255598163426,
        }

        self.assertRasterFitsUnivar(
            "test_basic",
            reference=reference_basic,
            precision=0.002,
        )

        self.assertModule(
            "r.geomorphon",
            flags="e",
            elevation=self.inele,
            forms="test_extended",
            search=20,
        )

        reference_extended = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.85362112067877,
            "stddev": 1.81621087978012,
        }

        self.assertRasterFitsUnivar(
            "test_extended",
            reference=reference_extended,
            precision=0.002,
        )

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_basic != test_extended, 1, null())",
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
        )

        self.assertModule(
            "r.geomorphon",
            flags="m",
            elevation=self.inele,
            forms="test_meters",
            search=30,
        )

        reference_meters = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.65599353551678,
            "stddev": 1.73140628723594,
        }

        self.assertRasterFitsUnivar(
            "test_meters",
            reference=reference_meters,
            precision=0.002,
        )

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_cells != test_meters, 1, null())",
        )
        stats_diff = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats_diff["n"]), 0)


class TestComparisonModes(TestCase):
    """Test different comparison modes for zenith/nadir line-of-sight"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        """Compute the anglev1 map once for use as a baseline in all tests"""
        cls.use_temp_region()
        cls.runModule("g.region", **region)
        cls.runModule(
            "r.geomorphon",
            elevation=cls.inele,
            forms="test_anglev1",
            search=10,
            comparison="anglev1",
        )

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", flags="f", type="raster", name="test_anglev1")
        cls.del_temp_region()

    def tearDown(self):
        """Remove test outputs"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=("test_anglev2", "test_anglev2_dist", "test_diff"),
        )

    def test_anglev1_mode(self):
        """Test anglev1 comparison mode (default) against reference statistics"""
        reference = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.81124214034999,
            "stddev": 1.77419698170854,
        }

        self.assertRasterFitsUnivar(
            "test_anglev1",
            reference=reference,
            precision=0.002,
        )

    def test_anglev2_mode(self):
        """Test that anglev2 comparison mode differs from anglev1"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_anglev2",
            search=10,
            comparison="anglev2",
        )

        reference_anglev2 = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.82252973409762,
            "stddev": 1.78012008833156,
        }

        self.assertRasterFitsUnivar(
            "test_anglev2",
            reference=reference_anglev2,
            precision=0.002,
        )

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_anglev1 != test_anglev2, 1, null())",
        )

        stats = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats["n"]), 0)

    def test_anglev2_distance_mode(self):
        """Test that anglev2_distance comparison mode differs from anglev1"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            forms="test_anglev2_dist",
            search=10,
            comparison="anglev2_distance",
        )

        # On this dataset, anglev2_distance produces the same result
        # as anglev2, but both differ from anglev1.
        reference_anglev2_dist = {
            "n": 158404,
            "null_cells": 1596,
            "min": 1,
            "max": 10,
            "mean": 5.82252973409762,
            "stddev": 1.78012008833156,
        }

        self.assertRasterFitsUnivar(
            "test_anglev2_dist",
            reference=reference_anglev2_dist,
            precision=0.002,
        )

        self.runModule(
            "r.mapcalc",
            expression="test_diff = if(test_anglev1 != test_anglev2_dist, 1, null())",
        )

        stats = gs.parse_command("r.univar", flags="g", map="test_diff")
        self.assertGreater(float(stats["n"]), 0)


class TestProfileFormat(TestCase):
    """Test profile output format parameter"""

    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", **region)
        cls.test_easting = (region["e"] + region["w"]) / 2
        cls.test_northing = (region["n"] + region["s"]) / 2

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
