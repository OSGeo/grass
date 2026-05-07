#!/usr/bin/env python

############################################################################
#
# NAME:      interactivemap_test.py
#
# AUTHOR:    Caitlin Haedrich (caitlin dot haedrich gmail com)
#
# PURPOSE:   This is a test script for grass.jupyter's InteractiveMap
#
# COPYRIGHT: (C) 2021 by Caitlin Haedrich and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
#############################################################################


import unittest
from pathlib import Path

import grass.jupyter as gj
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


def can_import_folium():
    """Test folium import to see if test can be run."""
    try:
        import folium  # noqa

        return True
    except ImportError:
        return False


def can_import_ipyleaflet():
    """Test ipyleaflet import to see if test can be run."""
    try:
        import ipyleaflet  # noqa

        return True
    except ImportError:
        return False


class TestDisplay(TestCase):
    # Setup variables
    files = []

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region"""
        # to not override mapset's region (which might be used by other tests)
        cls.use_temp_region()
        # cls.runModule or self.runModule is used for general module calls
        # we'll use the elevation raster as a test display
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """
        Remove the PNG file created after testing with "filename =" option.
        This is executed after each test run.
        """
        for f in self.files:
            f = Path(f)
            f.unlink(missing_ok=True)

    @unittest.skipIf(not can_import_folium(), "Cannot import folium")
    def test_basic(self):
        # Create InteractiveMap
        interactive_map = gj.InteractiveMap()
        interactive_map.add_raster("elevation")
        interactive_map.add_vector("roadsmajor")
        interactive_map.show()

    @unittest.skipIf(not can_import_folium(), "Cannot import folium")
    def test_save_as_html(self):
        # Create InteractiveMap
        interactive_map = gj.InteractiveMap()
        interactive_map.add_vector("roadsmajor")
        filename = "InteractiveMap_test.html"
        self.files.append(filename)
        interactive_map.save(filename)
        self.assertFileExists(filename)

    @unittest.skipIf(not can_import_ipyleaflet(), "Cannot import ipyleaflet")
    def test_query_button(self):
        # Create InteractiveMap with ipyleaflet backend
        interactive_map = gj.InteractiveMap(map_backend="ipyleaflet")
        interactive_map.add_raster("elevation")
        button = interactive_map.setup_query_interface()
        self.assertIsNotNone(interactive_map._controllers[button].query_raster((0, 0)))

    @unittest.skipIf(not can_import_ipyleaflet(), "Cannot import ipyleaflet")
    def test_draw(self):
        """Test the draw_computational_region method."""
        # Create InteractiveMap
        interactive_map = gj.InteractiveMap(map_backend="ipyleaflet")
        button = interactive_map.setup_drawing_interface()
        interactive_map._controllers[button].activate()
        self.assertIsNotNone(interactive_map._controllers[button].save_button_control)

    @unittest.skipIf(not can_import_ipyleaflet(), "Cannot import ipyleaflet")
    def test_draw_computational_region(self):
        """Test the draw_computational_region method."""
        # Create InteractiveMap
        interactive_map = gj.InteractiveMap(map_backend="ipyleaflet")
        button = interactive_map.setup_computational_region_interface()
        interactive_map._controllers[button].activate()
        self.assertIsNotNone(interactive_map._controllers[button].save_button_control)

    @unittest.skipIf(not can_import_folium(), "Cannot import folium")
    def test_nonexistent_raster_error(self):
        """Test that adding non-existent raster raises clear error"""
        interactive_map = gj.InteractiveMap()
        with self.assertRaises(ValueError) as context:
            interactive_map.add_raster("xelevation")
        msg = str(context.exception).lower()
        self.assertIn("raster", msg)
        self.assertIn("xelevation", msg)

    @unittest.skipIf(not can_import_folium(), "Cannot import folium")
    def test_nonexistent_vector_error(self):
        """Test that adding non-existent vector raises clear error"""
        interactive_map = gj.InteractiveMap()
        with self.assertRaises(ValueError) as context:
            interactive_map.add_vector("xroadsmajor")
        msg = str(context.exception).lower()
        self.assertIn("vector", msg)
        self.assertIn("xroadsmajor", msg)


if __name__ == "__main__":
    test()
