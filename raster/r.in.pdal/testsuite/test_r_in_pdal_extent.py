import os
import sys
import pathlib
import unittest
import shutil

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.script as gs
from grass.tools import Tools

# points.csv
# X,Y,Z
# 637510,222000,100
# 637505,222012,101
# 637512,222006,102
# 637500,222004,103
# 637503,222009,104
# 637503,222009,106
# 637504,222008,106
# 637511,222001,107
# 637510,222002,108
# 637504,222006,109

# pipeline_3358.json
# {
#   "pipeline": [
#     {
#       "type": "readers.text",
#       "filename": "points.csv"
#     },
#     {
#       "type": "writers.las",
#       "filename": "data/points_3358.las",
#       "a_srs": "EPSG:3358"
#     }
#   ]

# pipeline_3358_6346.json
# [
#     {
#     "type": "readers.las",
#     "filename": "data/points_3358.las"
#     },
#     {
#     "type": "filters.reprojection",
#     "out_srs": "EPSG:6346"
#     },
#     {
#     "type": "writers.las",
#     "filename": "data/points_6346.las"
#     }
# ]

# pdal pipeline pipeline_3358.json
# pdal pipeline pipeline_3358_6346.json


class ExtentTest(TestCase):
    """Testing extent with reprojection"""

    output = "pdal_output"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)

        cls.data_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "data")
        cls.point_file_6346 = os.path.join(cls.data_dir, "points_6346.las")
        cls.point_file_3358 = os.path.join(cls.data_dir, "points_3358.las")

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created by the import

        This is executed after each test run.
        """
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(self.output),
        )

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_reprojection_extent(self):
        """Test extent is automatically reprojected from epsg 6346"""
        tools = Tools()
        extent = tools.r_in_pdal(input=self.point_file_6346, flags="g").text
        extent = gs.parse_key_val(extent, vsep=" ", val_type=float)
        expected = {
            "n": 222012.882582,
            "s": 222000.743357,
            "e": 637511.895399,
            "w": 637499.697426,
            "b": 100,
            "t": 109,
        }
        if sys.platform == "darwin":
            delta = 0.3
        else:
            delta = 1e-6
        for key, expected_value in expected.items():
            with self.subTest(key=key):
                self.assertAlmostEqual(extent[key], expected_value, delta=delta)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_no_reprojection_needed_extent(self):
        """Test extent matches input 3358 data"""
        tools = Tools()
        extent = tools.r_in_pdal(input=self.point_file_3358, flags="g").text
        extent = gs.parse_key_val(extent, vsep=" ", val_type=float)
        self.assertAlmostEqual(extent["n"], 222012, places=6)
        self.assertAlmostEqual(extent["s"], 222000, places=6)
        self.assertAlmostEqual(extent["e"], 637512, places=6)
        self.assertAlmostEqual(extent["w"], 637500, places=6)
        self.assertAlmostEqual(extent["b"], 100, places=6)
        self.assertAlmostEqual(extent["t"], 109, places=6)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_override_extent(self):
        """Test extent matches input 3358 data"""
        tools = Tools()
        extent = tools.r_in_pdal(input=self.point_file_3358, flags="go").text
        extent = gs.parse_key_val(extent, vsep=" ", val_type=float)
        self.assertAlmostEqual(extent["n"], 222012, places=6)
        self.assertAlmostEqual(extent["s"], 222000, places=6)
        self.assertAlmostEqual(extent["e"], 637512, places=6)
        self.assertAlmostEqual(extent["w"], 637500, places=6)
        self.assertAlmostEqual(extent["b"], 100, places=6)
        self.assertAlmostEqual(extent["t"], 109, places=6)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_no_reprojection_needed_flags_e(self):
        """Test 3358 data is imported correctly"""
        tools = Tools()
        tools.g_region(n=1, s=0, e=1, w=0, res=1)
        tools.r_in_pdal(input=self.point_file_3358, output=self.output, flags="e")
        self.assertRasterExists(self.output)
        rinfo = tools.r_info(map=self.output, flags="gr", format="json")
        self.assertAlmostEqual(222012, rinfo["north"])
        self.assertAlmostEqual(222000, rinfo["south"])
        self.assertAlmostEqual(637512, rinfo["east"])
        self.assertAlmostEqual(637500, rinfo["west"])
        # The extent shows min=100, that point is filtered out
        # because it's at the edge, this may not be ideal behavior
        self.assertAlmostEqual(101, rinfo["min"])
        self.assertAlmostEqual(109, rinfo["max"])

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_reprojection_needed_flags_en(self):
        """Test 6346 data is imported correctly and test n flag"""
        tools = Tools()
        tools.g_region(n=1, s=0, e=1, w=0, res=1)
        tools.r_in_pdal(
            input=self.point_file_6346, output=self.output, resolution=2, flags="en"
        )
        self.assertRasterExists(self.output)
        rinfo = tools.r_info(map=self.output, flags="g", format="json")
        region = tools.g_region(flags="p", format="json")
        univar = tools.r_univar(map=self.output, format="json")
        self.assertAlmostEqual(region["north"], rinfo["north"])
        self.assertAlmostEqual(region["south"], rinfo["south"])
        self.assertAlmostEqual(region["east"], rinfo["east"])
        self.assertAlmostEqual(region["west"], rinfo["west"])
        self.assertAlmostEqual(2, rinfo["nsres"])
        self.assertAlmostEqual(2, rinfo["ewres"])
        # No points here are filtered out
        self.assertAlmostEqual(univar["min"], 100)
        self.assertAlmostEqual(univar["max"], 109)


if __name__ == "__main__":
    test()
