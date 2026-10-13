from grass.exceptions import OpenError
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.raster import RasterRow, RasterSegment


class RasterTestCase(TestCase):
    """Base class creating the test raster map and region"""

    name = None
    # Univariate statistics of the map created in setUpClass.
    univar = {"n": 16, "null_cells": 0, "min": 11, "max": 44, "sum": 440}

    @classmethod
    def setUpClass(cls):
        """Create test raster map and region"""
        cls.use_temp_region()
        cls.addClassCleanup(cls.del_temp_region)
        cls.runModule("g.region", n=40, s=0, e=40, w=0, res=10)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.name} = row() + (10.0 * col())",
            overwrite=True,
        )
        cls.addClassCleanup(
            cls.runModule, "g.remove", flags="f", type="raster", name=cls.name
        )


class RasterRowTestCase(RasterTestCase):
    name = "RasterRowTestCase_map"

    def test_type(self):
        r = RasterRow(self.name)
        r.open(mode="r")
        self.assertTrue(r.mtype, "DCELL")
        r.close()

    def test_isopen(self):
        r = RasterRow(self.name)
        self.assertFalse(r.is_open())
        r.open(mode="r")
        self.assertTrue(r.is_open())
        r.close()
        self.assertFalse(r.is_open())

    def test_name(self):
        r = RasterRow(self.name)
        r.open(mode="r")
        self.assertEqual(r.name, self.name)
        fullname = "{name}@{mapset}".format(name=r.name, mapset=r.mapset)
        self.assertEqual(r.fullname(), fullname)
        r.close()

    def test_exist(self):
        notexist = RasterRow(self.name + "notexist")
        self.assertFalse(notexist.exist())
        exist = RasterRow(self.name)
        self.assertTrue(exist.exist())

    def test_open_r(self):
        notexist = RasterRow(self.name + "notexist")
        with self.assertRaises(OpenError):
            # raster does not exist
            notexist.open(mode="r")
        r = RasterRow(self.name)
        r.open(mode="r", mtype="FCELL")
        # ignore the mtype if is open in read mode
        self.assertEqual(r.mtype, "DCELL")
        r.close()

    def test_open_w(self):
        r = RasterRow(self.name)
        with self.assertRaises(OpenError):
            # raster type is not defined!
            r.open(mode="w")
        with self.assertRaises(OpenError):
            # raster already exist
            r.open(mode="w", mtype="DCELL")
        # open in write mode and overwrite
        r.open(mode="w", mtype="DCELL", overwrite=True)
        self.assertTrue(r.mtype, "DCELL")
        r.close()

    def test_row_range(self):
        r = RasterRow(self.name)
        with self.assertRaises(IndexError):
            # Map is not open yet
            r[1]
        with self.assertRaises(IndexError):
            # Index is out of range
            r.open()
            r[9999]
        r.close()


class RasterSegmentTestCase(RasterTestCase):
    name = "RasterSegmentTestCase_map"

    def copy_test_map(self, name):
        """Copy the test map so that a test can overwrite it"""
        self.runModule("g.copy", raster=f"{self.name},{name}")
        self.addCleanup(self.runModule, "g.remove", flags="f", type="raster", name=name)
        return name

    def test_open_r_not_exist(self):
        notexist = RasterSegment(self.name + "notexist")
        with self.assertRaises(OpenError):
            notexist.open(mode="r")

    def test_open_w_already_exists(self):
        r = RasterSegment(self.name)
        with self.assertRaises(OpenError):
            r.open(mode="w", mtype="DCELL", overwrite=False)
        self.assertFalse(r.is_open())
        self.assertRasterFitsUnivar(self.name, reference=self.univar)

    def test_open_rw_already_exists(self):
        r = RasterSegment(self.name)
        with self.assertRaises(OpenError):
            r.open(mode="rw", overwrite=False)
        self.assertFalse(r.is_open())
        self.assertRasterFitsUnivar(self.name, reference=self.univar)

    def test_open_w_overwrite(self):
        r = RasterSegment(self.copy_test_map("test_open_w_overwrite_map"))
        r.open(mode="w", mtype="DCELL", overwrite=True)
        self.assertTrue(r.is_open())
        r.close()

    def test_open_rw_overwrite(self):
        r = RasterSegment(self.copy_test_map("test_open_rw_overwrite_map"))
        r.open(mode="rw", overwrite=True)
        self.assertTrue(r.is_open())
        r.close()


if __name__ == "__main__":
    test()
