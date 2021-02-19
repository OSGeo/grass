# -*- coding: utf-8
import numpy as np
import unittest

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.raster import raster2numpy_img
from grass.pygrass.gis.region import Region
from grass.script.core import tempfile

has_PyQt4 = False
try:
    from PyQt4.QtCore import *
    from PyQt4.QtGui import *
    has_PyQt4 = True
except:
    pass


class RasterRowImgTestCase(TestCase):

    name = "RasterRowImgTestCase_map"

    @classmethod
    def setUpClass(cls):
        """Create test raster map and region"""
        cls.use_temp_region()
        cls.runModule("g.region", n=60, s=0, e=40, w=0, res=0.1)
        cls.runModule("r.mapcalc",
            expression="%s = if(row() >= 10 && row() <= 60, null(), row()  + (10.0 * col()))" % (cls.name),
            overwrite=True)
        cls.runModule("r.colors", map=cls.name, color="elevation")

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='raster',
                      name=cls.name)
        cls.del_temp_region()

    @unittest.skipIf(has_PyQt4 is False, "Require PyQt4")
    def test_resampling_to_QImg_1(self):

        region = Region()
        region.from_rast(self.name)
        region.cols = 320
        region.rows = 240
        region.adjust()

        tmpfile = tempfile(False)
        tmpfile = tmpfile + ".png"

        a = raster2numpy_img(self.name, region)

        image = QImage(a.data, region.cols, region.rows,
                       QImage.Format_ARGB32)
        # image.save("data/a.png")
        image.save(tmpfile)
        self.assertFilesEqualMd5(tmpfile, "data/a.png")

    @unittest.skipIf(has_PyQt4 is False, "Require PyQt4")
    def test_resampling_to_QImg_2(self):

        region = Region()
        region.from_rast(self.name)
        region.cols = 640
        region.rows = 480
        region.adjust()

        tmpfile = tempfile(False)
        tmpfile = tmpfile + ".png"

        # With array as argument
        array = np.ndarray((region.rows * region.cols * 4), np.uint8)

        raster2numpy_img(rastname=self.name, region=region,
                         color="ARGB", array=array)

        image = QImage(array.data,
                       region.cols, region.rows, QImage.Format_ARGB32)
        # image.save("data/b.png")
        image.save(tmpfile)
        self.assertFilesEqualMd5(tmpfile, "data/b.png")

    @unittest.skipIf(has_PyQt4 is False, "Require PyQt4")
    def test_resampling_to_QImg_large(self):

        region = Region()
        region.from_rast(self.name)
        region.cols = 4000
        region.rows = 3000
        region.adjust()

        tmpfile = tempfile(False)
        tmpfile = tmpfile + ".png"

        # With array as argument
        array = np.ndarray((region.rows * region.cols * 4), np.uint8)

        raster2numpy_img(rastname=self.name, region=region,
                         color="ARGB", array=array)

        image = QImage(array.data,
                       region.cols, region.rows, QImage.Format_ARGB32)
        # image.save("data/c.png")
        image.save(tmpfile)
        self.assertFilesEqualMd5(tmpfile, "data/c.png")

    @unittest.skipIf(has_PyQt4 is False, "Require PyQt4")
    def test_resampling_to_QImg_3(self):

        region = Region()
        region.from_rast(self.name)
        region.cols = 400
        region.rows = 300
        region.adjust()

        tmpfile = tempfile(False)
        tmpfile = tmpfile + ".png"

        # With array as argument
        array = np.ndarray((region.rows * region.cols * 4), np.uint8)

        raster2numpy_img(rastname=self.name, region=region,
                         color="RGB", array=array)

        image = QImage(array.data,
                       region.cols, region.rows, QImage.Format_RGB32)
        # image.save("data/d.png")
        image.save(tmpfile)
        self.assertFilesEqualMd5(tmpfile, "data/d.png")

    @unittest.skipIf(has_PyQt4 is False, "Require PyQt4")
    def test_resampling_to_QImg_4(self):

        region = Region()
        region.from_rast(self.name)
        region.cols = 400
        region.rows = 300
        region.adjust()

        tmpfile = tempfile(False)
        tmpfile = tmpfile + ".png"

        array = raster2numpy_img(rastname=self.name, region=region,
                                 color="RGB")

        image = QImage(array.data,
                       region.cols, region.rows, QImage.Format_RGB32)
        # image.save("data/e.png")
        image.save(tmpfile)
        self.assertFilesEqualMd5(tmpfile, "data/e.png")

    def test_resampling_to_numpy_img_1(self):

        region = Region()
        region.ewres = 10
        region.nsres = 10
        region.adjust(rows=True, cols=True)

        a = raster2numpy_img(self.name, region)

        self.assertEqual(len(a), region.rows * region.cols * 4)

    def test_resampling_to_numpy_img_2(self):

        region = Region()
        region.ewres = 1
        region.nsres = 1
        region.adjust(rows=True, cols=True)

        a = raster2numpy_img(self.name, region)

        self.assertEqual(len(a), region.rows * region.cols * 4)

    def test_resampling_to_numpy_img_3(self):

        region = Region()
        region.ewres = 0.4
        region.nsres = 0.4
        region.adjust(rows=True, cols=True)

        a = raster2numpy_img(self.name, region, color="GRAY1")

        self.assertEqual(len(a), region.rows * region.cols * 1)

    def test_resampling_to_numpy_img_4(self):

        region = Region()
        region.ewres = 0.1
        region.nsres = 0.1
        region.adjust(rows=True, cols=True)

        a = raster2numpy_img(self.name, region, color="GRAY2")

        self.assertEqual(len(a), region.rows * region.cols * 1)

if __name__ == '__main__':
    test()
