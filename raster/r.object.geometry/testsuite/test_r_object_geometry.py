"""
Name:      r_object_geometry_test
Purpose:   This script is to demonstrate a unit test for r.object.geometry
           module.
"""

import json
import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.gunittest.gmodules import SimpleModule

testraster1 = """\
north:   250000
south:   200000
east:    670000
west:    610000
rows:    4
cols:    4

1 1 2 2
1 1 2 2
3 3 2 2
3 3 2 2
"""


class TestObjectGeometryPixel(TestCase):
    """Test case for object geometry module"""

    # Setup variables to be used for outputs
    test_objects1 = "test_objects1"
    output_file_pixel = "output_file_pixel.csv"

    def setUp(self):
        """Imports test raster(s), ensures expected computational region and setup"""
        self.runModule(
            "r.in.ascii",
            input="-",
            type="CELL",
            stdin_=testraster1,
            output=self.test_objects1,
        )
        self.use_temp_region()
        self.runModule("g.region", raster=self.test_objects1)

    def tearDown(self):
        """Remove the outputs created from the object geometry module and the temporary region
        This is executed after each test run.
        """
        if os.path.isfile(self.output_file_pixel):
            os.remove(self.output_file_pixel)
        self.del_temp_region()
        self.runModule("g.remove", flags="f", type="raster", name=self.test_objects1)

    def test_object_geometry_pixel(self):
        """Test to see if the outputs are created and are correct in pixel units"""
        # run the object geometry module
        self.assertModule(
            "r.object.geometry", input=self.test_objects1, output=self.output_file_pixel
        )
        # check to see if output file exists
        self.assertFileExists(self.output_file_pixel, msg="Output file does not exist")
        # check if the output file is equal to the reference file
        self.assertFilesEqualMd5(
            self.output_file_pixel,
            "data/file_pixel.csv",
            msg="Output file is not equal to reference file",
        )

    def test_object_geometry_json(self):
        """Test json format output"""
        reference = [
            {
                "category": 1,
                "area": 4,
                "perimeter": 8,
                "compact_circle": 1.1283791670955126,
                "compact_square": 1,
                "fd": 2.999459154496928,
                "mean_x": 625000,
                "mean_y": 237500,
            },
            {
                "category": 2,
                "area": 8,
                "perimeter": 12,
                "compact_circle": 1.1968268412042982,
                "compact_square": 0.94280904158206347,
                "fd": 2.3898313512153728,
                "mean_x": 655000,
                "mean_y": 225000,
            },
            {
                "category": 3,
                "area": 4,
                "perimeter": 8,
                "compact_circle": 1.1283791670955126,
                "compact_square": 1,
                "fd": 2.999459154496928,
                "mean_x": 625000,
                "mean_y": 212500,
            },
        ]
        module = SimpleModule(
            "r.object.geometry", input=self.test_objects1, format="json"
        )
        self.runModule(module)
        data = json.loads(module.outputs.stdout)
        self.assertCountEqual(reference, data)


class TestObjectGeometryMeter(TestCase):
    """Test case for object geometry module"""

    # Setup variables to be used for outputs
    test_objects1 = "test_objects1"
    output_file_meter = "output_file_meter.csv"

    @classmethod
    def setUpClass(cls):
        """Imports test raster(s), ensures expected computational region and setup"""
        cls.runModule(
            "r.in.ascii",
            input="-",
            type="CELL",
            stdin_=testraster1,
            output=cls.test_objects1,
        )
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.test_objects1)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created from the object geometry module

        This is executed after each test run.
        """
        if os.path.isfile(self.output_file_meter):
            os.remove(self.output_file_meter)
        self.runModule("g.remove", flags="f", type="raster", name=self.test_objects1)

    def test_object_geometry_meter(self):
        """Test to see if the outputs are created and are correct in meter units"""
        # run the object geometry module
        self.assertModule(
            "r.object.geometry",
            input=self.test_objects1,
            output=self.output_file_meter,
            flags="m",
        )
        # check to see if output file exists
        self.assertFileExists(self.output_file_meter, msg="Output file does not exist")
        # check if the output file is equal to the reference file
        self.assertFilesEqualMd5(
            self.output_file_meter,
            "data/file_meter.csv",
            msg="Output file is not equal to reference file",
        )


if __name__ == "__main__":
    test()
