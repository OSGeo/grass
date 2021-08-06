#!/usr/bin/env python

############################################################################
#
# NAME:      display_test
#
# AUTHOR:    Caitlin Haedrich (caitlin dot haedrich gmail com)
#
# PURPOSE:   This is a test script for grass.jupyter's display module
#
# COPYRIGHT: (C) 2021 by Caitlin Haedrich and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
#############################################################################

# Dependencies
import grass.jupyter as gj
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import unittest
import os
from pathlib import Path

# Helper function
def can_import_folium():
    try:
        import folium
        return True
    except ImportError:
        return False

# Tests
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
            Path(f).unlink(missing_ok=True)

    @unittest.skipIf(can_import_folium() is False, "Cannot import folium")
    def test_defaults(self):
        """Test that GrassRenderer can create a map with default settings."""
        # Create a map with default inputs
        grass_renderer = gj.GrassRenderer()
        # Adding vectors and rasters to the map
        grass_renderer.run("d.rast", map="elevation")
        grass_renderer.run("d.vect", map="roadsmajor")
        # Assert image exists
        self.assertFileExists(grass_renderer._filename)

    @unittest.skipIf(can_import_folium() is False, "Cannot import folium")
    def test_filename(self):
        """Test that GrassRenderer creates maps with unique filenames."""
        # Create map with unique filename
        grass_renderer = gj.GrassRenderer(filename="test_filename.png")
        # Add files to self for cleanup later
        self.files.append("test_filename.png")
        self.files.append("test_filename.grass_vector_legend")
        # Add a vector and a raster to the map
        grass_renderer.run("d.rast", map="elevation")
        grass_renderer.run("d.vect", map="roadsmajor")
        # Assert image exists
        self.assertFileExists("test_filename.png")

    @unittest.skipIf(can_import_folium() is False, "Cannot import folium")
    def test_hw(self):
        """Test that GrassRenderer creates maps with unique height and widths."""
        # Create map with height and width parameters
        grass_renderer = gj.GrassRenderer(width=400, height=400)
        # Add just a vector (for variety here)
        grass_renderer.run("d.vect", map="roadsmajor")

    @unittest.skipIf(can_import_folium() is False, "Cannot import folium")
    def test_env(self):
        """Test that we can hand an environment to GrassRenderer."""
        # Create map with environment parameter
        grass_renderer = gj.GrassRenderer(env=os.environ.copy())
        # Add just a raster (again for variety)
        grass_renderer.run("d.rast", map="elevation")

    @unittest.skipIf(can_import_folium() is False, "Cannot import folium")
    def test_text(self):
        """Test that we can set a unique text_size in GrassRenderer."""
        # Create map with unique text_size parameter
        grass_renderer = gj.GrassRenderer(text_size=10)
        # Add a vector and a raster
        grass_renderer.run("d.vect", map="roadsmajor")
        grass_renderer.run("d.rast", map="elevation")

    @unittest.skipIf(can_import_folium() is False, "Cannot import folium")
    def test_shortcut(self):
        """Test that we can use display shortcuts with __getattr__."""
        # Create map
        grass_renderer = gj.GrassRenderer()
        # Add raster to map with shortcut
        grass_renderer.d_rast(map="elevation")


if __name__ == "__main__":
    test()
