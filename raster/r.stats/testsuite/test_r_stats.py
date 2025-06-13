#!/usr/bin/env python3

"""
MODULE:    Test of r.stats

AUTHOR(S): Nishant Bansal <nishant.bansal.282003@gmail.com>

PURPOSE:   Test that verifies the generated area statistics for a raster map.

COPYRIGHT: (C) 2025 by Nishant Bansal and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.main import test


class TestRStats(TestCase):
    """Test case for r.stats"""

    test_raster_1 = "test_raster_1"
    test_raster_2 = "test_raster_2"
    float_raster = "float_raster"

    test_label_1 = "1:trees\n2:3:water\n"
    test_label_2 = "1:trees\n2:4:buildings\n"
    float_label = (
        "0:0.5:0 slope\n"
        "0.5:1:1 degrees\n"
        "1:1.5:2 degrees\n"
        "1.5:2:3 degrees\n"
        "2:2.5:4 degrees\n"
        "2.5:3:5 degrees\n"
        "3.5:4:6 degrees\n"
        "4:4.5:7 degrees\n"
    )

    @classmethod
    def setUpClass(cls):
        """Set up test environment"""
        cls.runModule("g.region", s=0, n=100, w=0, e=100, res=20)

        cls.runModule(
            "r.mapcalc",
            expression=(f"{cls.test_raster_1} = if(col() < 3, col(), 2)"),
        )
        cls.runModule(
            "r.category",
            map=cls.test_raster_1,
            separator=":",
            rules="-",
            stdin=cls.test_label_1,
        )

        cls.runModule(
            "r.mapcalc",
            expression=(f"{cls.test_raster_2} = if(col() < 5, col(), null())"),
        )
        cls.runModule(
            "r.category",
            map=cls.test_raster_2,
            separator=":",
            rules="-",
            stdin=cls.test_label_2,
        )

        cls.runModule(
            "r.mapcalc",
            expression=(f"{cls.float_raster} = if(col() < 5, col() / 2., 4.5)"),
        )
        cls.runModule(
            "r.category",
            map=cls.float_raster,
            separator=":",
            rules="-",
            stdin=cls.float_label,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up test environment"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[cls.test_raster_1, cls.test_raster_2, cls.float_raster],
        )

    def test_percentage_flag_output(self):
        """Verify that r.stats with the -p flag returns correct percentage values with pipe separator."""
        rstats_module = SimpleModule(
            "r.stats", input=self.test_raster_1, flags="p", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = ["1|21.74%", "2|86.96%"]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_cross_tabulation_output(self):
        """Verify that r.stats with the -c flag produces correct cross-tabulation for multiple raster inputs."""
        rstats_module = SimpleModule(
            "r.stats",
            input=[self.test_raster_1, self.test_raster_2],
            flags="c",
            separator="comma",
        )
        self.assertModule(rstats_module)

        expected_output = ["1,1,5", "2,2,5", "2,3,5", "2,4,5", "2,*,5"]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_area_calculation_flag_output(self):
        """Verify that r.stats with the -a flag returns correct area calculations using pipe as a separator."""
        rstats_module = SimpleModule(
            "r.stats", input=self.test_raster_1, flags="a", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = ["1|2000.000000", "2|8000.000000"]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_label_output_flag(self):
        """Verify that r.stats with the -l flag returns category labels correctly using newline as the separator."""
        rstats_module = SimpleModule(
            "r.stats", input=self.float_raster, flags="l", separator="\n"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "0.5-0.515686",
            "from 1 degrees to 1 degrees",
            "0.986275-1.001961",
            "from 1 degrees to 2 degrees",
            "1.488235-1.503922",
            "from 2 degrees to 3 degrees",
            "1.990196-2.005882",
            "from 3 degrees to 4 degrees",
            "4.484314-4.5",
            "from 7 degrees to 7 degrees",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_grid_coordinates_output(self):
        """Verify that r.stats with the -g flag produces correct grid coordinates using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats", input=self.test_raster_1, flags="g", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "10|90|1",
            "30|90|2",
            "50|90|2",
            "70|90|2",
            "90|90|2",
            "10|70|1",
            "30|70|2",
            "50|70|2",
            "70|70|2",
            "90|70|2",
            "10|50|1",
            "30|50|2",
            "50|50|2",
            "70|50|2",
            "90|50|2",
            "10|30|1",
            "30|30|2",
            "50|30|2",
            "70|30|2",
            "90|30|2",
            "10|10|1",
            "30|10|2",
            "50|10|2",
            "70|10|2",
            "90|10|2",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_cell_coordinates_output(self):
        """Verify that r.stats with the -x flag outputs row, column, and category values using pipe as a separator."""
        rstats_module = SimpleModule(
            "r.stats", input=self.test_raster_1, flags="x", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1|1|1",
            "2|1|2",
            "3|1|2",
            "4|1|2",
            "5|1|2",
            "1|2|1",
            "2|2|2",
            "3|2|2",
            "4|2|2",
            "5|2|2",
            "1|3|1",
            "2|3|2",
            "3|3|2",
            "4|3|2",
            "5|3|2",
            "1|4|1",
            "2|4|2",
            "3|4|2",
            "4|4|2",
            "5|4|2",
            "1|5|1",
            "2|5|2",
            "3|5|2",
            "4|5|2",
            "5|5|2",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_area_statistics_output(self):
        """Verify that r.stats with the -A and -a flags outputs area statistics using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats",
            input=self.float_raster,
            flags="Aa",
            separator="pipe",
        )
        self.assertModule(rstats_module)

        expected_output = [
            "0.507843|2000.000000",
            "0.994118|2000.000000",
            "1.496078|2000.000000",
            "1.998039|2000.000000",
            "4.492157|2000.000000",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_raw_index_area_output(self):
        """Verify that r.stats with -r and -a flags outputs raw indexes and corresponding area using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats",
            input=self.float_raster,
            flags="ra",
            separator="pipe",
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1|2000.000000",
            "32|2000.000000",
            "64|2000.000000",
            "96|2000.000000",
            "255|2000.000000",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_cats_range_output(self):
        """Verify that r.stats with -C and -l flags computes cats ranges and outputs labels, using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats",
            input=self.float_raster,
            flags="Cl",
            separator="pipe",
        )
        self.assertModule(rstats_module)

        expected_output = [
            "0.5-1|1 degrees",
            "1-1.5|2 degrees",
            "2-2.5|4 degrees",
            "4-4.5|7 degrees",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_integer_category_counts_output(self):
        """Verify that r.stats with -i and -c flags outputs integer-rounded category values and their counts using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats",
            input=self.float_raster,
            flags="ic",
            separator="pipe",
        )
        self.assertModule(rstats_module)

        expected_output = ["1|10", "2|10", "5|5"]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_multiple_raster_inputs_with_stats_output(self):
        """Verify r.stats with multiple raster inputs using -nacp flags and tab separator."""
        rstats_module = SimpleModule(
            "r.stats",
            input=[self.test_raster_1, self.test_raster_2],
            flags="nacp",
            separator="tab",
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1\t1\t2000.000000\t5\t33.33%",
            "2\t2\t2000.000000\t5\t33.33%",
            "2\t3\t2000.000000\t5\t33.33%",
            "2\t4\t2000.000000\t5\t33.33%",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_per_cell_label_output(self):
        """Test r.stats with -1ln flags to output one cell per line."""
        rstats_module = SimpleModule(
            "r.stats",
            input=[self.test_raster_1, self.test_raster_2],
            flags="1ln",
            separator="pipe",
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1|trees|1|trees",
            "2|water|2|buildings",
            "2|water|3|buildings",
            "2|water|4|buildings",
            "1|trees|1|trees",
            "2|water|2|buildings",
            "2|water|3|buildings",
            "2|water|4|buildings",
            "1|trees|1|trees",
            "2|water|2|buildings",
            "2|water|3|buildings",
            "2|water|4|buildings",
            "1|trees|1|trees",
            "2|water|2|buildings",
            "2|water|3|buildings",
            "2|water|4|buildings",
            "1|trees|1|trees",
            "2|water|2|buildings",
            "2|water|3|buildings",
            "2|water|4|buildings",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)


if __name__ == "__main__":
    test()
