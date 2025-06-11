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

    @classmethod
    def setUpClass(cls):
        """Set up temporary computational region"""
        cls.use_temp_region()
        cls.runModule("g.region", rows=5, cols=5)

    @classmethod
    def tearDownClass(cls):
        """Clean up temporary computational region"""
        cls.del_temp_region()

    def test_percentage_flag_output(self):
        """Verify that r.stats with the -p flag returns correct percentage values with pipe separator."""
        rstats_module = SimpleModule(
            "r.stats", input="towns", flags="p", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1|15.00%",
            "2|10.00%",
            "4|15.00%",
            "5|70.00%",
            "6|15.00%",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_cross_tabulation_output(self):
        """Verify that r.stats with the -c flag produces correct cross-tabulation for multiple raster inputs."""
        rstats_module = SimpleModule(
            "r.stats",
            input=["landclass96", "zipcodes", "geology_30m"],
            flags="c",
            separator="comma",
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1,27511,405,1",
            "1,27529,270,1",
            "1,27601,270,1",
            "1,27603,217,1",
            "1,27604,270,1",
            "1,27606,217,1",
            "3,27603,270,2",
            "3,27606,217,1",
            "3,27607,217,1",
            "4,27518,405,1",
            "5,27518,405,1",
            "5,27529,270,1",
            "5,27603,270,2",
            "5,27605,262,1",
            "5,27606,217,3",
            "5,27606,262,1",
            "5,27606,405,1",
            "5,27607,217,1",
            "5,27607,766,1",
            "5,27610,270,1",
            "6,27606,217,1",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_area_calculation_flag_output(self):
        """Verify that r.stats with the -a flag returns correct area calculations using pipe as a separator."""
        rstats_module = SimpleModule(
            "r.stats", input="towns", flags="a", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1|24301610.280000",
            "2|16201073.520000",
            "4|24301610.280000",
            "5|113407514.640000",
            "6|24301610.280000",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_label_output_flag(self):
        """Verify that r.stats with the -l flag returns category labels correctly using newline as the separator."""
        rstats_module = SimpleModule(
            "r.stats", input="slope", flags="l", nsteps=10, separator="\n"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "0-3.868939",
            "from zero slope to 4 degrees",
            "3.868939-7.737878",
            "from 4 degrees to 8 degrees",
            "7.737878-11.606818",
            "from 8 degrees to 12 degrees",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_grid_coordinates_output(self):
        """Verify that r.stats with the -g flag produces correct grid coordinates using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats", input="towns", flags="g", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "631479.1|227176.35|6",
            "634477.3|227176.35|6",
            "637475.5|227176.35|6",
            "640473.7|227176.35|4",
            "643471.9|227176.35|4",
            "631479.1|224474.55|1",
            "634477.3|224474.55|5",
            "637475.5|224474.55|5",
            "640473.7|224474.55|5",
            "643471.9|224474.55|4",
            "631479.1|221772.75|1",
            "634477.3|221772.75|5",
            "637475.5|221772.75|5",
            "640473.7|221772.75|5",
            "643471.9|221772.75|5",
            "631479.1|219070.95|1",
            "634477.3|219070.95|5",
            "637475.5|219070.95|5",
            "640473.7|219070.95|5",
            "643471.9|219070.95|2",
            "631479.1|216369.15|5",
            "634477.3|216369.15|5",
            "637475.5|216369.15|5",
            "640473.7|216369.15|5",
            "643471.9|216369.15|2",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_cell_coordinates_output(self):
        """Verify that r.stats with the -x flag outputs row, column, and category values using pipe as a separator."""
        rstats_module = SimpleModule(
            "r.stats", input="towns", flags="x", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1|1|6",
            "2|1|6",
            "3|1|6",
            "4|1|4",
            "5|1|4",
            "1|2|1",
            "2|2|5",
            "3|2|5",
            "4|2|5",
            "5|2|4",
            "1|3|1",
            "2|3|5",
            "3|3|5",
            "4|3|5",
            "5|3|5",
            "1|4|1",
            "2|4|5",
            "3|4|5",
            "4|4|5",
            "5|4|2",
            "1|5|5",
            "2|5|5",
            "3|5|5",
            "4|5|5",
            "5|5|2",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_area_statistics_sorted_output(self):
        """Verify that r.stats with the -A and -a flags outputs area statistics with values sorted in descending order using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats", input="elevation", flags="Aa", separator="pipe", sort="desc"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "122.943725|16201073.520000",
            "102.398408|16201073.520000",
            "105.954329|8100536.760000",
            "81.853092|8100536.760000",
            "92.520852|8100536.760000",
            "122.15352|8100536.760000",
            "101.213102|8100536.760000",
            "100.817999|8100536.760000",
            "118.5976|8100536.760000",
            "131.635974|8100536.760000",
            "116.226987|8100536.760000",
            "94.496363|8100536.760000",
            "92.915955|8100536.760000",
            "143.884144|8100536.760000",
            "96.076772|8100536.760000",
            "94.101261|8100536.760000",
            "92.12575|8100536.760000",
            "127.28985|8100536.760000",
            "135.586997|8100536.760000",
            "117.412294|8100536.760000",
            "110.695555|8100536.760000",
            "73.951047|8100536.760000",
            "86.199216|8100536.760000",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_raw_index_area_output(self):
        """Verify that r.stats with -r and -a flags outputs raw indexes and corresponding area, sorted in descending order using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats", input="elevation", flags="ra", separator="pipe", sort="desc"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "171|16201073.520000",
            "119|16201073.520000",
            "128|8100536.760000",
            "67|8100536.760000",
            "94|8100536.760000",
            "169|8100536.760000",
            "116|8100536.760000",
            "115|8100536.760000",
            "160|8100536.760000",
            "193|8100536.760000",
            "154|8100536.760000",
            "99|8100536.760000",
            "95|8100536.760000",
            "224|8100536.760000",
            "103|8100536.760000",
            "98|8100536.760000",
            "93|8100536.760000",
            "182|8100536.760000",
            "203|8100536.760000",
            "157|8100536.760000",
            "140|8100536.760000",
            "47|8100536.760000",
            "78|8100536.760000",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_cats_range_output(self):
        """Verify that r.stats with -C and -l flags computes cats ranges and outputs labels, using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats",
            input="slope",
            flags="Cl",
            separator="pipe",
        )
        self.assertModule(rstats_module)

        expected_output = [
            "10.5-11.5|11 degrees",
            "6.5-7.5|7 degrees",
            "5.5-6.5|6 degrees",
            "4.5-5.5|5 degrees",
            "3.5-4.5|4 degrees",
            "2.5-3.5|3 degrees",
            "1.5-2.5|2 degrees",
            "0.5-1.5|1 degree",
            "0-0.5|zero slope",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_integer_category_counts_output(self):
        """Verify that r.stats with -i and -c flags outputs integer-rounded category values and their counts using pipe separator."""
        rstats_module = SimpleModule(
            "r.stats", input="elevation", flags="ic", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "74|1",
            "82|1",
            "86|1",
            "92|1",
            "93|2",
            "94|2",
            "96|1",
            "101|2",
            "102|1",
            "103|1",
            "106|1",
            "111|1",
            "116|1",
            "117|1",
            "118|1",
            "122|1",
            "123|2",
            "127|1",
            "132|1",
            "136|1",
            "144|1",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_multiple_raster_inputs_with_stats_output(self):
        """Verify r.stats with multiple raster inputs using -nacp flags and tab separator."""
        rstats_module = SimpleModule(
            "r.stats", input=["towns", "urban"], flags="nacp", separator="tab"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "1\t55\t24301610.280000\t3\t18.75%",
            "2\t55\t16201073.520000\t2\t12.50%",
            "4\t55\t24301610.280000\t3\t18.75%",
            "5\t55\t89105904.360000\t11\t68.75%",
            "6\t55\t24301610.280000\t3\t18.75%",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)

    def test_per_cell_label_output(self):
        """Test r.stats with -1ln flags to output one line per cell."""
        rstats_module = SimpleModule(
            "r.stats", input=["towns", "urban"], flags="1ln", separator="pipe"
        )
        self.assertModule(rstats_module)

        expected_output = [
            "6|RALEIGH-WEST|55|",
            "6|RALEIGH-WEST|55|",
            "6|RALEIGH-WEST|55|",
            "4|RALEIGH-CITY|55|",
            "4|RALEIGH-CITY|55|",
            "1|CARY|55|",
            "5|RALEIGH-SOUTH|55|",
            "5|RALEIGH-SOUTH|55|",
            "5|RALEIGH-SOUTH|55|",
            "4|RALEIGH-CITY|55|",
            "1|CARY|55|",
            "5|RALEIGH-SOUTH|55|",
            "5|RALEIGH-SOUTH|55|",
            "5|RALEIGH-SOUTH|55|",
            "1|CARY|55|",
            "5|RALEIGH-SOUTH|55|",
            "2|GARNER|55|",
            "5|RALEIGH-SOUTH|55|",
            "5|RALEIGH-SOUTH|55|",
            "5|RALEIGH-SOUTH|55|",
            "5|RALEIGH-SOUTH|55|",
            "2|GARNER|55|",
        ]

        actual_output = rstats_module.outputs.stdout.splitlines()
        self.assertEqual(actual_output, expected_output)


if __name__ == "__main__":
    test()
