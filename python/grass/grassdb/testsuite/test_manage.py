# MODULE:    Test of grass.grassdb.manage
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Test of managing the GRASS database/location/mapset structure
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Tests of grass.grassdb.manage"""

from pathlib import Path

from grass.grassdb.manage import MapsetPath, resolve_mapset_path, split_mapset_path
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import call_module
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows


class TestMapsetPath(TestCase):
    """Check that object can be constructed"""

    def test_mapset_from_path_object(self):
        """Check that path is correctly stored"""
        path = "does/not/exist/"
        location_name = "test_location_A"
        mapset_name = "test_mapset_1"
        full_path = Path(path) / location_name / mapset_name
        mapset_path = MapsetPath(
            path=full_path, directory=path, location=location_name, mapset=mapset_name
        )
        # Paths are currently stored as is (not resolved).
        self.assertEqual(mapset_path.directory, path)
        self.assertEqual(mapset_path.location, location_name)
        self.assertEqual(mapset_path.mapset, mapset_name)
        self.assertEqual(mapset_path.path, Path(path) / location_name / mapset_name)

    @xfail_windows
    def test_mapset_from_str(self):
        """Check with path from str and database directory as Path"""
        path = "does/not/exist"
        location_name = "test_location_A"
        mapset_name = "test_mapset_1"
        full_path = Path(path) / location_name / mapset_name
        mapset_path = MapsetPath(
            path=str(full_path),
            directory=Path(path),
            location=location_name,
            mapset=mapset_name,
        )
        # Paths are currently stored as is (not resolved).
        self.assertEqual(mapset_path.directory, path)
        self.assertEqual(mapset_path.location, location_name)
        self.assertEqual(mapset_path.mapset, mapset_name)
        self.assertEqual(mapset_path.path, Path(path) / location_name / mapset_name)


class TestSplitMapsetPath(TestCase):
    """Check that split works with different parameters"""

    @xfail_windows
    def test_split_path(self):
        """Check that pathlib.Path is correctly split"""
        ref_db = "does/not/exist"
        ref_location = "test_location_A"
        ref_mapset = "test_mapset_1"
        path = Path(ref_db) / ref_location / ref_mapset
        new_db, new_location, new_mapset = split_mapset_path(path)
        self.assertEqual(new_db, ref_db)
        self.assertEqual(new_location, ref_location)
        self.assertEqual(new_mapset, ref_mapset)

    @xfail_windows
    def test_split_str(self):
        """Check that path as str is correctly split"""
        ref_db = "does/not/exist"
        ref_location = "test_location_A"
        ref_mapset = "test_mapset_1"
        path = Path(ref_db) / ref_location / ref_mapset
        new_db, new_location, new_mapset = split_mapset_path(str(path))
        self.assertEqual(new_db, ref_db)
        self.assertEqual(new_location, ref_location)
        self.assertEqual(new_mapset, ref_mapset)

    @xfail_windows
    def test_split_str_trailing_slash(self):
        """Check that path as str with a trailing slash is correctly split"""
        ref_db = "does/not/exist"
        ref_location = "test_location_A"
        ref_mapset = "test_mapset_1"
        path = Path(ref_db) / ref_location / ref_mapset
        new_db, new_location, new_mapset = split_mapset_path(str(path) + "/")
        self.assertEqual(new_db, ref_db)
        self.assertEqual(new_location, ref_location)
        self.assertEqual(new_mapset, ref_mapset)


class TestResolveMapsetPath(TestCase):
    """Check expected results for current mapset and for a non-existent one"""

    def test_default_mapset_exists(self):
        """Check that default mapset is found for real path/location.

        The location (or mapset) may not exist, but exist in the test.
        """
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        loc_name = call_module("g.gisenv", get="LOCATION_NAME").strip()
        mapset_path = resolve_mapset_path(path=db_path, location=loc_name)
        self.assertEqual(mapset_path.mapset, "PERMANENT")

    def test_default_mapset_does_not_exist(self):
        """Check that default mapset is found for non-existent path/location.

        The location (or mapset) do not exist.
        """
        mapset_path = resolve_mapset_path(
            path="does/not/exist", location="does_not_exit"
        )
        self.assertEqual(mapset_path.mapset, "PERMANENT")

    def test_default_mapset_with_path(self):
        """Check that default mapset is found for path.

        This requires the location (with default mapset) to exists.
        """
        db_path = call_module("g.gisenv", get="GISDBASE").strip()
        loc_name = call_module("g.gisenv", get="LOCATION_NAME").strip()
        mapset_path = resolve_mapset_path(path=Path(db_path) / loc_name)
        self.assertEqual(mapset_path.mapset, "PERMANENT")

    def test_mapset_from_parts(self):
        """Check that a non-existing path is correctly constructed."""
        path = "does/not/exist"
        location_name = "test_location_A"
        mapset_name = "test_mapset_1"
        mapset_path = resolve_mapset_path(
            path=path, location=location_name, mapset=mapset_name
        )
        self.assertEqual(mapset_path.directory, str(Path(path).resolve()))
        self.assertEqual(mapset_path.location, location_name)
        self.assertEqual(mapset_path.mapset, mapset_name)
        self.assertEqual(
            mapset_path.path, Path(path).resolve() / location_name / mapset_name
        )

    def test_mapset_from_path(self):
        """Check that a non-existing path is correctly parsed."""
        path = "does/not/exist/"
        location_name = "test_location_A"
        mapset_name = "test_mapset_1"
        full_path = str(Path(path) / location_name / mapset_name)
        mapset_path = resolve_mapset_path(path=full_path)
        self.assertEqual(mapset_path.directory, str(Path(path).resolve()))
        self.assertEqual(mapset_path.location, location_name)
        self.assertEqual(mapset_path.mapset, mapset_name)
        self.assertEqual(
            mapset_path.path, Path(path).resolve() / location_name / mapset_name
        )


if __name__ == "__main__":
    test()
