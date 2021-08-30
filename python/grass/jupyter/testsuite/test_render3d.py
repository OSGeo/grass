#!/usr/bin/env python

############################################################################
#
# NAME:      Test 3D renderer
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Test script for grass.jupyter's Grass3dRenderer
#
# COPYRIGHT: (C) 2021 by Vaclav Petras and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
#############################################################################

"""Test of 3D renderer"""

import os
import unittest
import sys
from pathlib import Path
import grass.jupyter as gj
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


def can_import_ipython():
    """Return True if IPython can be imported, False otherwise"""
    try:
        import IPython

        return True
    except ImportError:
        return False


def can_import_pyvirtualdisplay():
    """Return True if pyvirtualdisplay can be imported, False otherwise"""
    try:
        import pyvirtualdisplay

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
        """Check that default settings work"""
        renderer = gj.Grass3dRenderer()
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(renderer.filename)

    def test_filename(self):
        """Check that custom filename works"""
        custom_filename = "test_filename.png"
        renderer = gj.Grass3dRenderer(filename=custom_filename)
        # Add files to self for cleanup later
        self.files.append(custom_filename)
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(custom_filename)

    def test_hw(self):
        """Check that custom width and height works"""
        renderer = gj.Grass3dRenderer(width=200, height=400)
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(renderer.filename)

    def test_overlay(self):
        """Check that overlay works"""
        renderer = gj.Grass3dRenderer()
        renderer.render(elevation_map="elevation", color_map="elevation")
        renderer.overlay.d_legend(raster="elevation", at=(60, 97, 87, 92))
        self.assertFileExists(renderer.filename)

    @unittest.skipIf(
        not can_import_pyvirtualdisplay(), "Cannot import PyVirtualDisplay"
    )
    def test_pyvirtualdisplay_backend(self):
        """Check that pyvirtualdisplay backend works"""
        renderer = gj.Grass3dRenderer(screen_backend="pyvirtualdisplay")
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(renderer.filename)

    def test_shortcut_error(self):
        """Check that wrong screen backend fails"""
        with self.assertRaisesRegex(ValueError, "does_not_exist"):
            gj.Grass3dRenderer(screen_backend="does_not_exist")

    @unittest.skipIf(not can_import_ipython(), "Cannot import IPython")
    def test_image_creation(self):
        """Check that show() works"""
        renderer = gj.Grass3dRenderer()
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertTrue(renderer.show(), "Failed to create IPython Image object")


if __name__ == "__main__":
    test()
