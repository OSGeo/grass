# -*- coding: utf-8 -*-
"""
Created on Thu Jun 19 14:13:53 2014

@author: pietro
"""
import sys
import unittest
import numpy as np

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.lib.vector as libvect
from grass.script.core import run_command

from grass.pygrass.vector import VectorTopo


class GeometryAttrsTestCase(TestCase):

    tmpname = "GeometryAttrsCase_map"

    @classmethod
    def setUpClass(cls):

        from grass.pygrass import utils
        utils.create_test_vector_map(cls.tmpname)

        cls.vect = None
        cls.vect = VectorTopo(cls.tmpname)
        cls.vect.open('r')
        cls.c_mapinfo = cls.vect.c_mapinfo

        cls.attrs = cls.vect[1].attrs

    @classmethod
    def tearDownClass(cls):
        cls.attrs = None
        if cls.vect is not None:
            cls.vect.close()
            cls.c_mapinfo = None

        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='vector',
                      name=cls.tmpname)

    def test_getitem(self):
        """Test __getitem__ magic method"""
        self.assertEqual(self.attrs['name'], u'point')
        self.assertEqual(self.attrs['value'], 1.0)
        self.assertTupleEqual(self.attrs['name', 'value'], (u'point', 1.0))

        with self.assertRaises(ValueError) as cm:
            self.attrs['not_existing_column_name']

        self.assertTrue(u"not_existing_column_name" in str(cm.exception))


    def test_setitem(self):
        """Test __setitem__ magic method"""
        newname = 'setitem_point_1'
        newvalue = 100.0
        newpairs = ('setitem_point_2', 1000.)

        self.attrs.__setitem__('name', newname)
        self.assertEqual(self.attrs['name'], newname)
        self.attrs.__setitem__('value', newvalue)
        self.assertEqual(self.attrs['value'], newvalue)
        self.attrs.__setitem__(('name', 'value'), newpairs)
        self.assertEqual(self.attrs['name', 'value'], newpairs)


if __name__ == '__main__':
    test()
