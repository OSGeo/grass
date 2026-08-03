"""Test of r.timestamp basic functionality

@author Jal Patel

@copyright 2026 by Jal Patel and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module

from grass.script.core import tempname


class TestRTimestamp(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression=f"{cls.map} = 1")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

    def tearDown(self):
        # Reset to a clean, timestamp-less map so tests don't depend on order
        call_module("r.timestamp", map=self.map, date="none")

    def test_no_timestamp_by_default(self):
        """Reading a map with no timestamp set should fail"""
        self.assertModuleFail("r.timestamp", map=self.map)

    def test_set_and_read_absolute(self):
        self.assertModule("r.timestamp", map=self.map, date="15 sep 1987")
        output = call_module("r.timestamp", map=self.map)
        self.assertEqual(output.strip(), "15 Sep 1987")

    def test_set_and_read_range(self):
        self.assertModule("r.timestamp", map=self.map, date="15 sep 1987/20 feb 1988")
        output = call_module("r.timestamp", map=self.map)
        self.assertEqual(output.strip(), "15 Sep 1987 / 20 Feb 1988")

    def test_overwrite_timestamp(self):
        self.assertModule("r.timestamp", map=self.map, date="15 sep 1987")
        self.assertModule("r.timestamp", map=self.map, date="1 jan 2000")
        output = call_module("r.timestamp", map=self.map)
        self.assertEqual(output.strip(), "1 Jan 2000")

    def test_remove_timestamp(self):
        self.assertModule("r.timestamp", map=self.map, date="15 sep 1987")
        self.assertModule("r.timestamp", map=self.map, date="none")
        self.assertModuleFail("r.timestamp", map=self.map)

    def test_invalid_date_format(self):
        self.assertModuleFail("r.timestamp", map=self.map, date="not a date")

    def test_nonexistent_map_read(self):
        self.assertModuleFail("r.timestamp", map="this_map_does_not_exist_xyz")

    def test_nonexistent_map_write(self):
        self.assertModuleFail(
            "r.timestamp", map="this_map_does_not_exist_xyz", date="15 sep 1987"
        )


if __name__ == "__main__":
    test()
