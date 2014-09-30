# -*- coding: utf-8 -*-
"""
Created on Wed Jun 18 17:21:42 2014

@author: pietro
"""
from grass.gunittest import TestCase, test

from grass.pygrass.vector import VectorTopo


class VectorTopoTestCase(TestCase):

    vname = "points_of_interest"

    def test_getitem_slice(self):
        """Test that getitem handle correctly the slice starting from 1"""
        vcoords = ((646341.7386813264, 218873.73056803632),
                   (637772.0990144431, 218842.80557760992))
        with VectorTopo(self.vname, mode="r") as vect:
            coords = tuple([pnt.coords() for pnt in vect[:3]])
            self.assertTupleEqual(vcoords, coords)
            coords = tuple([pnt.coords() for pnt in vect[1:3]])
            self.assertTupleEqual(vcoords, coords)

    def test_getitem_raise(self):
        """Test that getitem raise a value error if the key is not
        an integer or a slice"""
        with VectorTopo(self.vname, mode="r") as vect:
            with self.assertRaises(ValueError):
                vect['value']


if __name__ == '__main__':
    test()
