"""
Created on Mon Sep 15 17:09:40 2014

@author: lucadelu
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.raster import RasterRow
from grass.pygrass.raster.history import History
from grass.script.utils import decode


class RasterHistoryTestCate(TestCase):
    name = "RasterCategoryTestCase_map"

    @classmethod
    def setUpClass(cls):
        """Create test raster map and region"""
        cls.use_temp_region()
        cls.runModule("g.region", n=40, s=0, e=40, w=0, res=10)
        cls.runModule(
            "r.mapcalc",
            expression="%s = row() + (10 * col())" % (cls.name),
            overwrite=True,
        )
        cls.runModule(
            "r.support",
            map=cls.name,
            title="A test map",
            history="Generated by r.mapcalc",
            description="This is a test map",
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.name)
        cls.del_temp_region()

    def testHistory(self):
        r = RasterRow(self.name)
        r.open("r")
        hist = r.hist

        self.assertEqual(decode(hist.title), self.name)
        self.assertEqual(decode(hist.keyword), "This is a test map")

        hist1 = History(self.name)
        hist1.read()

        self.assertEqual(decode(hist1.title), self.name)
        self.assertEqual(decode(hist1.keyword), "This is a test map")

        self.assertEqual(hist, hist1)
        self.assertEqual(decode(hist.creator), decode(hist1.creator))
        hist1.creator = "Markus"
        self.assertNotEqual(decode(hist.creator), decode(hist1.creator))
        r.close()

        hist1.title = "No such title"
        hist1.keyword = "No such description"
        hist1.src1 = "No such source 1"
        hist1.src2 = "No such source 2"
        hist1.write()

        r.open("r")
        hist = r.hist

        self.assertEqual(decode(hist.title), "No such title")
        self.assertEqual(decode(hist.keyword), "No such description")
        self.assertEqual(decode(hist.creator), "Markus")
        self.assertEqual(decode(hist.creator), "Markus")
        self.assertEqual(decode(hist.src1), "No such source 1")
        self.assertEqual(decode(hist.src2), "No such source 2")
        r.close()


if __name__ == "__main__":
    test()
