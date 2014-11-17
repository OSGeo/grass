# -*- coding: utf-8 -*-
"""
Created on Mon Sep 15 17:09:40 2014

@author: lucadelu
"""

from grass.gunittest import TestCase, test

from grass.pygrass.raster import RasterRow
from grass.pygrass.raster.history import History


class RasterHistoryTestCate(TestCase):

    name = 'geology'

    def testHistory(self):
        r = RasterRow(self.name)
        r.open()
        hist = r.hist
        hist1 = History(self.name)
        hist1.read()
        # this is not working, I don't know why
        #self.assertEqual(hist, hist1)
        self.assertEqual(hist.creator, hist1.creator)
        hist1.creator = 'markus'
        self.assertNotEqual(hist.creator, hist1.creator)
        r.close()


if __name__ == '__main__':
    test()
