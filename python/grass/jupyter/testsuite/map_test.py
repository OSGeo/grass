#!/usr/bin/env python

############################################################################
#
# NAME:      map_test.py
#
# AUTHOR:    Caitlin Haedrich (caitlin dot haedrich gmail com)
#
# PURPOSE:   This is a test script for grass.jupyter's Map
#
# COPYRIGHT: (C) 2021 by Caitlin Haedrich and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
#############################################################################

import os
import sys
import unittest
from pathlib import Path

import grass.jupyter as gj
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


def can_import_ipython():
    """Test folium import to see if test can be run."""
    try:
        import IPython  # noqa

        return True
    except ImportError:
        return False


class TestMap(TestCase):
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
        """Test that Map can create a map with default settings."""
        # Create a map with default inputs
        grass_renderer = gj.Map()
        # Adding vectors and rasters to the map
        grass_renderer.run("d.rast", map="elevation")
        grass_renderer.run("d.vect", map="roadsmajor")
        # Make sure image was created
        self.assertFileExists(grass_renderer._filename)

    def test_filename(self):
        """Test that Map creates maps with unique filenames."""
        # Create map with unique filename
        custom_filename = "test_filename_provided.png"
        grass_renderer = gj.Map(filename=custom_filename)
        # Add files to self for cleanup later
        self.files.append(custom_filename)
        self.files.append(f"{custom_filename}.grass_vector_legend")
        # Add a vector and a raster to the map
        grass_renderer.run("d.rast", map="elevation")
        grass_renderer.run("d.vect", map="roadsmajor")
        # Make sure image was created
        self.assertFileExists(custom_filename)

    def test_filename_property(self):
        """Test of Map filename property."""
        # Create map with unique filename
        grass_renderer = gj.Map()
        grass_renderer.run("d.rast", map="elevation")
        # Make sure image was created
        self.assertFileExists(grass_renderer.filename)

    def test_save_file(self):
        """Test saving of file"""
        grass_renderer = gj.Map()
        # Add a vector and a raster to the map
        grass_renderer.run("d.rast", map="elevation")
        custom_filename = "test_filename_save.png"
        grass_renderer.save(custom_filename)
        # Add files to self for cleanup later
        self.files.append(custom_filename)
        # Make sure image was created
        self.assertFileExists(custom_filename)

    def test_hw(self):
        """Test that Map creates maps with custom height and widths."""
        # Create map with height and width parameters
        grass_renderer = gj.Map(width=400, height=400)
        # Add just a vector (for variety here)
        grass_renderer.run("d.vect", map="roadsmajor")

    def test_env(self):
        """Test that we can hand an environment to Map."""
        # Create map with environment parameter
        grass_renderer = gj.Map(env=os.environ.copy())
        # Add just a raster (again for variety)
        grass_renderer.run("d.rast", map="elevation")

    def test_text(self):
        """Test that we can set a unique text_size in Map."""
        # Create map with unique text_size parameter
        grass_renderer = gj.Map(text_size=10)
        grass_renderer.run("d.vect", map="roadsmajor")
        grass_renderer.run("d.rast", map="elevation")

    def test_shortcut(self):
        """Test that we can use display shortcuts with __getattr__."""
        # Create map
        grass_renderer = gj.Map()
        # Use shortcut
        grass_renderer.d_rast(map="elevation")
        grass_renderer.d_vect(map="roadsmajor")

    def test_shortcut_error(self):
        """Test that passing an incorrect attribute raises
        appropriate error"""
        # Create map
        grass_renderer = gj.Map()
        # Pass bad shortcuts
        with self.assertRaisesRegex(AttributeError, "Module must begin with 'd_'"):
            grass_renderer.r_watersheds()
        with self.assertRaisesRegex(AttributeError, "d.module.does.not.exist"):
            grass_renderer.d_module_does_not_exist()

    @unittest.skipIf(not can_import_ipython(), "Cannot import IPython")
    def test_image_creation(self):
        """Test that show() returns None."""
        # Create map
        grass_renderer = gj.Map()
        grass_renderer.d_rast(map="elevation")
        self.assertIsNone(grass_renderer.show(), "Failed to open PNG image")


if __name__ == "__main__":
    test()
