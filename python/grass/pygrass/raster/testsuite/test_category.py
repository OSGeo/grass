"""
Created on Mon Sep 15 17:09:40 2014

@author: lucadelu
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.pygrass.raster import RasterRow
from grass.pygrass.raster.category import Category
from grass.script.core import tempfile


class RasterCategoryTestCase(TestCase):
    name = "RasterCategoryTestCase_map"

    @classmethod
    def setUpClass(cls):
        """Create test raster map and region"""
        cls.use_temp_region()
        cls.runModule("g.region", n=40, s=0, e=40, w=0, res=10)
        cls.runModule(
            "r.mapcalc",
            expression="%s = row() + (10.0 * col())" % (cls.name),
            overwrite=True,
        )
        cls.runModule(
            "r.support",
            map=cls.name,
            title="A test map",
            history="Generated by r.mapcalc",
            description="This is a test map",
        )
        cats = """11:A
                12:B
                13:C
                14:D
                21:E
                22:F
                23:G
                24:H
                31:I
                32:J
                33:K
                34:L
                41:M
                42:n
                43:O
                44:P"""

        cls.runModule("r.category", rules="-", map=cls.name, stdin_=cats, separator=":")

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.name)
        cls.del_temp_region()

    def testCategory(self):
        r = RasterRow(self.name)
        r.open()
        cats = r.cats
        cats1 = Category(self.name)
        cats1.read()
        self.assertEqual(cats, cats1)
        r.close()

    def testFirstCat(self):
        cat0 = ("A", 11, None)
        cat7 = ("H", 24, None)
        cat15 = ("P", 44, None)
        cats = Category(self.name)
        cats.read()
        self.assertEqual(cats[0], cat0)
        self.assertEqual(cats[7], cat7)
        self.assertEqual(cats[15], cat15)

    def testWrite(self):
        tmpfile = tempfile(False)
        cats = Category(self.name)
        cats.read()
        cats.write_rules(tmpfile)
        self.assertFilesEqualMd5(tmpfile, "data/geology_cats")


if __name__ == "__main__":
    test()
