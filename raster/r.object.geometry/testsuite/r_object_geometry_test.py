"""
Name:      r_object_geometry_test
Purpose:   This script is to demonstrate a unit test for r.object.geometry
           module.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

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
        if os.path.isfile(self.output_file_pixel):
            os.remove(self.output_file_pixel)
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
