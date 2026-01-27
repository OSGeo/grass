"""
Name:       r.geomorphon tests
Purpose:    Tests r.geomorphon input parsing.
            Uses NC Basic data set.

Author:     Luca Delucchi, Markus Neteler
Copyright:  (C) 2017 by Luca Delucchi, Markus Neteler and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command
import grass.script as gs


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
        """Test r.geomorphon with elevation data"""
        self.runModule(
            "r.geomorphon", elevation=self.inele, forms=self.outele, search=10
        )
        self.assertRasterExists(self.outele)
        # Check that various landform types are present
        stats = read_command("r.stats", flags="n", input=self.outele)
        self.assertIn("1", stats)  # flat should be present

    def test_sint(self):
        """Test r.geomorphon with synthetic data"""
        self.runModule(
            "r.geomorphon", elevation=self.insint, forms=self.outsint, search=10
        )
        self.assertRasterExists(self.outsint)
        # Check that output is generated
        stats = read_command("r.stats", flags="n", input=self.outsint)
        self.assertIn("1", stats)  # flat should be present


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

    def test_ternary_output(self):
        """Test ternary pattern output"""
        self.assertModule(
            "r.geomorphon", elevation=self.inele, ternary="test_ternary", search=10
        )
        self.assertRasterExists("test_ternary")

    def test_intensity_output(self):
        """Test geometry output (intensity)"""
        self.assertModule(
            "r.geomorphon", elevation=self.inele, intensity="test_intensity", search=10
        )
        self.assertRasterExists("test_intensity")

    def test_elongation_output(self):
        """Test shape output (elongation)"""
        self.assertModule(
            "r.geomorphon",
            elevation=self.inele,
            elongation="test_elongation",
            search=10,
        )
        self.assertRasterExists("test_elongation")


class TestFlags(TestCase):
    inele = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        outputs = ["test_extended"]
        existing = [o for o in outputs if gs.find_file(name=o, element="cell")["file"]]
        if existing:
            self.runModule("g.remove", flags="f", type="raster", name=existing)

    def test_extended_flag(self):
        """Test extended form correction flag"""
        self.assertModule(
            "r.geomorphon",
            flags="e",
            elevation=self.inele,
            forms="test_extended",
            search=10,
        )
        self.assertRasterExists("test_extended")


if __name__ == "__main__":
    test()
