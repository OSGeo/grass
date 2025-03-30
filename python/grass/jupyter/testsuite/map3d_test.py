#!/usr/bin/env python

############################################################################
#
# NAME:      Test 3D renderer
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Test script for grass.jupyter's Map3D
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
import sys
import unittest
from pathlib import Path

import grass.jupyter as gj
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows


def can_import_ipython():
    """Return True if IPython can be imported, False otherwise"""
    try:
        # pylint: disable=import-outside-toplevel,unused-import
        import IPython  # noqa: F401

        return True
    except ImportError:
        return False


def can_import_pyvirtualdisplay():
    """Return True if pyvirtualdisplay can be imported, False otherwise"""
    try:
        # pylint: disable=import-outside-toplevel,unused-import
        import pyvirtualdisplay  # noqa: F401

        return True
    except ImportError:
        return False


class TestMap3D(TestCase):
    """Test Map3D"""

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
        """After each run, remove the created files if exist"""
        for file in self.files:
            file = Path(file)
            if sys.version_info < (3, 8):
                try:
                    os.remove(file)
                except FileNotFoundError:
                    pass
            else:
                file.unlink(missing_ok=True)

    @xfail_windows
    def test_defaults(self):
        """Check that default settings work"""
        renderer = gj.Map3D()
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(renderer.filename)

    @xfail_windows
    def test_filename(self):
        """Check that custom filename works"""
        custom_filename = "test_filename.png"
        renderer = gj.Map3D(filename=custom_filename)
        # Add files to self for cleanup later
        self.files.append(custom_filename)
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(custom_filename)

    @xfail_windows
    def test_hw(self):
        """Check that custom width and height works"""
        renderer = gj.Map3D(width=200, height=400)
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(renderer.filename)

    @xfail_windows
    def test_overlay(self):
        """Check that overlay works"""
        renderer = gj.Map3D()
        renderer.render(elevation_map="elevation", color_map="elevation")
        renderer.overlay.d_legend(raster="elevation", at=(60, 97, 87, 92))
        self.assertFileExists(renderer.filename)

    @unittest.skipIf(
        not can_import_pyvirtualdisplay(), "Cannot import PyVirtualDisplay"
    )
    def test_pyvirtualdisplay_backend(self):
        """Check that pyvirtualdisplay backend works"""
        renderer = gj.Map3D(screen_backend="pyvirtualdisplay")
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertFileExists(renderer.filename)

    def test_shortcut_error(self):
        """Check that wrong screen backend fails"""
        with self.assertRaisesRegex(ValueError, "does_not_exist"):
            gj.Map3D(screen_backend="does_not_exist")

    @unittest.skipIf(not can_import_ipython(), "Cannot import IPython")
    def test_image_creation(self):
        """Check that show() works"""
        renderer = gj.Map3D()
        renderer.render(elevation_map="elevation", color_map="elevation")
        self.assertIsNone(renderer.show(), "Failed to create IPython Image object")


if __name__ == "__main__":
    test()
