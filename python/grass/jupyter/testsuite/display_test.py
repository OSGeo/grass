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
import os

# The module performs as expected if:
#    1) it can create an instance of GrassRenderer with different heights
#       widths, filenames, environments and text_sizes.
#    2) it can call a 'd.*' GRASS module
#    3) write and display an image

# TODO
# After temporary files have been added for png images, update tearDown and
# test_GrassRenderer.


# Tests
class TestDisplay(TestCase):
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

    @classmethod
    def tearDown(self):
        """
        Remove the PNG file created after testing with "filename =" option.
        This is executed after each test run.
        """
        os.remove("map.png")
        os.remove("test_env.png")
        os.remove("test_wh.png")

    def test_GrassRenderer(self):
        """Test that we can create GrassRenderer objects with different heights,
        widths, filenames and environments, and text sizes."""
        # First, we test the init method with various inputs
        self.mapdisplay_filename = gj.GrassRenderer()
        self.mapdisplay_env = gj.GrassRenderer(
            env=os.environ(), filename="test_env.png"
        )
        self.mapdisplay_wh = gj.GrassRenderer(
            height=400, width=600, filename="test_wh.png"
        )
        self.mapdisplay_text = gj.GrassRenderer(text_size=10, filename="test_text.png")

        # Then, we test the adding vectors and rasters to the map
        self.mapdisplay_filename.run("d.rast", map="elevation")
        self.mapdisplay_env.run("d.vect", map="roadsmajor")

        # Finally, we make sure the images exists and we can open them
        self.assertFileExists("map.png")
        self.assertFileExists("test_env.png")
        self.assertFileExists("test_wh.png")
        self.assertFileExists("test_text")


if __name__ == "__main__":
    test()
