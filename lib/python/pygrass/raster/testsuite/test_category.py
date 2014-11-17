# -*- coding: utf-8 -*-
"""
Created on Mon Sep 15 17:09:40 2014

@author: lucadelu
"""

from grass.gunittest import TestCase, test

from grass.pygrass.raster import RasterRow
from grass.pygrass.raster.category import Category
from grass.script.core import tempfile


class RasterCategoryTestCate(TestCase):

    name = 'geology'
    catsname = ''

    def testCategory(self):
        r = RasterRow(self.name)
        r.open()
        cats = r.cats
        cats1 = Category(self.name)
        cats1.read()
        # this is not working, I don't know why
        self.assertEqual(cats, cats1)
        r.close()

    def testFirstCat(self):
        cat0 = ('Zml', 1, None)
        cats = Category(self.name)
        cats.read()
        self.assertEqual(cats[0], cat0)

    def testWrite(self):
        tmpfile = tempfile(False)
        cats = Category(self.name)
        cats.read()
        cats.write_rules(tmpfile)
        self.assertFilesEqualMd5(tmpfile, 'data/geology_cats')


if __name__ == '__main__':
    test()
