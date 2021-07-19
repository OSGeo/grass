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
from grass.gunittest.utils import silent_rmtree
import os
import shutil

# The module performs as expected if:
#    1) it can create an instance of GrassRenderer with different heights
#       widths, filenames, environments and text_sizes.
#    2) it can call a 'd.*' GRASS module
#    3) write and display an image

# TODO
# 1. After temporary files have been added for png images, update tearDown and
# test_GrassRenderer tests
# 2. Add __getattr shortcut test after merge


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
        # silent_rmtree("/map.grass_vector_legend")
        # silent_rmtree("/test_filename.grass_vector_legend")
        # silent_rmtree("/map.png")
        # silent_rmtree("/test_filename.png")
        # shutil.rmtree(os.path.join(os.getcwd(),"map.png"))
        
        files = ["map.grass_vector_legend", 
                "test_filename.grass_vector_legend",
                "map.png",
                "test_filename.png"]
        for f in files:
            try:
                os.remove(os.path.join(os.getcwd(), f))
            except OSError:
                pass

    def test_GrassRenderer_defaults(self):
        """Test that GrassRenderer can create a map with default settings."""
        # Create a map with default inputs
        self.mapdisplay = gj.GrassRenderer()
        # Adding vectors and rasters to the map
        self.mapdisplay.run("d.rast", map="elevation")
        self.mapdisplay.run("d.vect", map="roadsmajor")
        # Assert image exists
        self.assertFileExists("map.png")

    def test_GrassRenderer_filename(self):
        """Test that GrassRenderer creates maps with unique filenames."""
        # Create map with unique filename
        self.mapdisplay = gj.GrassRenderer(filename="test_filename.png")
        # Add a vector and a raster to the map
        self.mapdisplay.run("d.rast", map="elevation")
        self.mapdisplay.run("d.vect", map="roadsmajor")
        # Assert image exists
        self.assertFileExists("test_filename.png")

    def test_GrassRenderer_hw(self):
        """Test that GrassRenderer creates maps with unique height and widths."""
        # Create map with height and width parameters
        self.mapdisplay = gj.GrassRenderer(width=400, height=400)
        # Add just a vector (for variety here)
        self.mapdisplay.run("d.vect", map="roadsmajor")
        # Assert image exists
        self.assertFileExists("map.png")

    def test_GrassRenderer_env(self):
        """Test that we can hand an environment to GrassRenderer."""
        # Create map with environment parameter
        self.mapdisplay = gj.GrassRenderer(env=os.environ.copy())
        # Add just a raster (again for variety)
        self.mapdisplay.run("d.rast", map="elevation")
        # Assert image exists
        self.assertFileExists("map.png")

    def test_GrassRenderer_text(self):
        """Test that we can set a unique text_size in GrassRenderer."""
        # Create map with unique text_size parameter
        self.mapdisplay = gj.GrassRenderer(text_size=10)
        # Add a vector and a raster
        self.mapdisplay.run("d.vect", map="roadsmajor")
        self.mapdisplay.run("d.rast", map="elevation")
        # Assert image exists
        self.assertFileExists("map.png")


if __name__ == "__main__":
    test()
