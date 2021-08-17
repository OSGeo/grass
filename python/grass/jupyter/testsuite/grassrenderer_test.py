#!/usr/bin/env python

############################################################################
#
# NAME:      grassrenderer_test.py
#
# AUTHOR:    Caitlin Haedrich (caitlin dot haedrich gmail com)
#
# PURPOSE:   This is a test script for grass.jupyter's GrassRenderer
#
# COPYRIGHT: (C) 2021 by Caitlin Haedrich and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
#############################################################################

import os
import unittest
import sys
from pathlib import Path
import grass.jupyter as gj
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


def can_import_ipython():
    """Test folium import to see if test can be run."""
    try:
        import IPython

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
            if sys.version_info < (3, 8):
                try:
                    os.remove(f)
                except FileNotFoundError:
                    pass
            else:
                f.unlink(missing_ok=True)

    def test_defaults(self):
        """Test that GrassRenderer can create a map with default settings."""
        # Create a map with default inputs
        grass_renderer = gj.GrassRenderer()
        # Adding vectors and rasters to the map
        grass_renderer.run("d.rast", map="elevation")
        grass_renderer.run("d.vect", map="roadsmajor")
        # Make sure image was created
        self.assertFileExists(grass_renderer._filename)

    def test_filename(self):
        """Test that GrassRenderer creates maps with unique filenames."""
        # Create map with unique filename
        custom_filename = "test_filename.png"
        grass_renderer = gj.GrassRenderer(filename=custom_filename)
        # Add files to self for cleanup later
        self.files.append(custom_filename)
        self.files.append(f"{custom_filename}.grass_vector_legend")
        # Add a vector and a raster to the map
        grass_renderer.run("d.rast", map="elevation")
        grass_renderer.run("d.vect", map="roadsmajor")
        # Make sure image was created
        self.assertFileExists(custom_filename)

    def test_hw(self):
        """Test that GrassRenderer creates maps with custom height and widths."""
        # Create map with height and width parameters
        grass_renderer = gj.GrassRenderer(width=400, height=400)
        # Add just a vector (for variety here)
        grass_renderer.run("d.vect", map="roadsmajor")

    def test_env(self):
        """Test that we can hand an environment to GrassRenderer."""
        # Create map with environment parameter
        grass_renderer = gj.GrassRenderer(env=os.environ.copy())
        # Add just a raster (again for variety)
        grass_renderer.run("d.rast", map="elevation")

    def test_text(self):
        """Test that we can set a unique text_size in GrassRenderer."""
        # Create map with unique text_size parameter
        grass_renderer = gj.GrassRenderer(text_size=10)
        grass_renderer.run("d.vect", map="roadsmajor")
        grass_renderer.run("d.rast", map="elevation")

    def test_shortcut(self):
        """Test that we can use display shortcuts with __getattr__."""
        # Create map
        grass_renderer = gj.GrassRenderer()
        # Use shortcut
        grass_renderer.d_rast(map="elevation")
        grass_renderer.d_vect(map="roadsmajor")

    def test_shortcut_error(self):
        """Test that passing an incorrect attribute raises
        appropriate error"""
        # Create map
        grass_renderer = gj.GrassRenderer()
        # Pass bad shortcuts
        with self.assertRaisesRegex(AttributeError, "Module must begin with 'd_'"):
            grass_renderer.r_watersheds()
        with self.assertRaisesRegex(AttributeError, "d.module.does.not.exist"):
            grass_renderer.d_module_does_not_exist()

    @unittest.skipIf(not can_import_ipython(), "Cannot import IPython")
    def test_image_creation(self):
        """Test that show() returns an image object."""
        # Create map
        grass_renderer = gj.GrassRenderer()
        grass_renderer.d_rast(map="elevation")
        self.assertTrue(grass_renderer.show(), "Failed to open PNG image")


if __name__ == "__main__":
    test()
