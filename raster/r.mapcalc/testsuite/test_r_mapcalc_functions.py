#!/usr/bin/env python3

############################################################################
#
# MODULE:        test_r_mapcalc_functions.py
# AUTHOR:        Saurabh Nxf
# PURPOSE:       Test math functions, conditionals, and operators in r.mapcalc
# COPYRIGHT:     (C) 2026 by Saurabh Nxf and the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestMathFunctions(TestCase):
    """Test mathematical functions in r.mapcalc"""

    output = "test_output"
    to_remove = []

    @classmethod
    def setUpClass(cls):
        """Create test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule("g.remove", flags="f", type="raster", name=cls.to_remove)

    def test_sqrt(self):
        """Test sqrt() function"""
        expression = f"{self.output} = sqrt(16)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 4, "range": 0}, precision=1e-6
        )

    def test_exp(self):
        """Test exp() function"""
        expression = f"{self.output} = exp(0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_log(self):
        """Test log() function"""
        expression = f"{self.output} = log(2.718281828)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=0.01
        )

    def test_sin(self):
        """Test sin() function"""
        expression = f"{self.output} = sin(0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 0, "range": 0}, precision=1e-6
        )

    def test_cos(self):
        """Test cos() function"""
        expression = f"{self.output} = cos(0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_abs(self):
        """Test abs() function"""
        expression = f"{self.output} = abs(-5)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 5, "range": 0}, precision=1e-6
        )


class TestConditionals(TestCase):
    """Test conditional statements in r.mapcalc"""

    input_map = "test_input"
    output = "test_output"
    to_remove = []

    @classmethod
    def setUpClass(cls):
        """Create test data"""
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)
        # Create input map with values 0-8
        cls.runModule("r.mapcalc", expression=f"{cls.input_map} = row() * col()")
        cls.to_remove.append(cls.input_map)

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule("g.remove", flags="f", type="raster", name=cls.to_remove)

    def test_if_simple(self):
        """Test simple if() statement"""
        expression = f"{self.output} = if({self.input_map} > 4, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.to_remove.append(self.output)
        # Values > 4 should be 1, others 0
        self.assertRasterExists(self.output)

    def test_if_nested(self):
        """Test nested if() statements"""
        expression = f"{self.output} = if({self.input_map} < 3, 1, if({self.input_map} < 6, 2, 3))"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)


class TestLogicalOperators(TestCase):
    """Test logical operators in r.mapcalc"""

    output = "test_output"
    to_remove = []

    @classmethod
    def setUpClass(cls):
        """Create test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule("g.remove", flags="f", type="raster", name=cls.to_remove)

    def test_and_operator(self):
        """Test && (AND) operator"""
        expression = f"{self.output} = if(1 && 1, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_or_operator(self):
        """Test || (OR) operator"""
        expression = f"{self.output} = if(0 || 1, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_not_operator(self):
        """Test ! (NOT) operator"""
        expression = f"{self.output} = if(!0, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )


class TestComparisonOperators(TestCase):
    """Test comparison operators in r.mapcalc"""

    output = "test_output"
    to_remove = []

    @classmethod
    def setUpClass(cls):
        """Create test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule("g.remove", flags="f", type="raster", name=cls.to_remove)

    def test_greater_than(self):
        """Test > operator"""
        expression = f"{self.output} = if(5 > 3, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_less_than(self):
        """Test < operator"""
        expression = f"{self.output} = if(3 < 5, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_equal(self):
        """Test == operator"""
        expression = f"{self.output} = if(5 == 5, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_not_equal(self):
        """Test != operator"""
        expression = f"{self.output} = if(5 != 3, 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )


class TestNullHandling(TestCase):
    """Test null value handling in r.mapcalc"""

    null_map = "test_null"
    output = "test_output"
    to_remove = []

    @classmethod
    def setUpClass(cls):
        """Create test data with nulls"""
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)
        # Create map with null values
        cls.runModule("r.mapcalc", expression=f"{cls.null_map} = null()")
        cls.to_remove.append(cls.null_map)

    @classmethod
    def tearDownClass(cls):
        """Clean up"""
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule("g.remove", flags="f", type="raster", name=cls.to_remove)

    def test_isnull(self):
        """Test isnull() function"""
        expression = f"{self.output} = if(isnull({self.null_map}), 1, 0)"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_null_creation(self):
        """Test null() function"""
        expression = f"{self.output} = null()"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterFitsUnivar(self.output, {"n": 0}, precision=1e-6)


if __name__ == "__main__":
    test()
