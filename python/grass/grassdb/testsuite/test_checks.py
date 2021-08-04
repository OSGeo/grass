# MODULE:    Test of grass.benchmark
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Basic tests of grass.benchmark"""

import os
from pathlib import Path

from grass.grassdb.checks import (dir_contains_location, get_list_of_locations,
                                  is_location_valid, is_mapset_valid,
                                  location_exists, mapset_exists)
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import call_module
from grass.gunittest.main import test


class TestWithCurrent(TestCase):
    """Tests that functions return expected result for the current mapset"""

    def test_valid_location(self):
        """Test that different parameter combinations work and return true"""
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        loc_name = call_module("g.gisenv", get="LOCATION_NAME").strip()
        self.assertTrue(is_location_valid(db_path, loc_name))
        self.assertTrue(is_location_valid(os.path.join(db_path, loc_name)))
        self.assertTrue(is_location_valid(Path(db_path), loc_name))
        self.assertTrue(is_location_valid(Path(db_path) / loc_name))

    def test_valid_mapset(self):
        """Test that different parameter combinations work and return true"""
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        loc_name = call_module("g.gisenv", get="LOCATION_NAME").strip()
        mapset_name = call_module("g.gisenv", get="MAPSET").strip()
        self.assertTrue(is_mapset_valid(db_path, loc_name, mapset_name))
        self.assertTrue(is_mapset_valid(os.path.join(db_path, loc_name, mapset_name)))
        self.assertTrue(is_mapset_valid(Path(db_path), loc_name, mapset_name))
        self.assertTrue(is_mapset_valid(Path(db_path) / loc_name / mapset_name))

    def test_location_exists(self):
        """Test that different parameter combinations work and return true"""
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        loc_name = call_module("g.gisenv", get="LOCATION_NAME").strip()
        self.assertTrue(location_exists(db_path, loc_name))
        self.assertTrue(location_exists(os.path.join(db_path, loc_name)))
        self.assertTrue(location_exists(Path(db_path), loc_name))
        self.assertTrue(location_exists(Path(db_path) / loc_name))

    def test_mapset_exists(self):
        """Test that different parameter combinations work and return true"""
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        loc_name = call_module("g.gisenv", get="LOCATION_NAME").strip()
        mapset_name = call_module("g.gisenv", get="MAPSET").strip()
        self.assertTrue(mapset_exists(db_path, loc_name, mapset_name))
        self.assertTrue(mapset_exists(os.path.join(db_path, loc_name, mapset_name)))
        self.assertTrue(mapset_exists(Path(db_path), loc_name, mapset_name))
        self.assertTrue(mapset_exists(Path(db_path) / loc_name / mapset_name))

    def test_dir_contains_location(self):
        """Test that different parameter combinations work and return true"""
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        self.assertTrue(dir_contains_location(db_path))
        self.assertTrue(dir_contains_location(Path(db_path)))

    def test_get_list_of_locations(self):
        """Test that different parameter combinations work and return true"""
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        current_loc_name = call_module("g.gisenv", get="LOCATION_NAME").strip()
        list_of_locations = get_list_of_locations(db_path)
        self.assertTrue(list_of_locations, msg="No locations in the current db found")
        self.assertIn(current_loc_name, list_of_locations)


if __name__ == "__main__":
    test()
