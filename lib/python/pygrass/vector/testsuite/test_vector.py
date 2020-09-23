# -*- coding: utf-8 -*-
"""
Created on Wed Jun 18 17:21:42 2014

@author: pietro
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import run_command
from grass.pygrass.vector import VectorTopo


class VectorTopoTestCase(TestCase):

    tmpname = "VectorTopoTestCase_map"

    @classmethod
    def setUpClass(cls):
        
        from grass.pygrass import utils
        utils.create_test_vector_map(cls.tmpname)
        
        cls.vect = None
        cls.vect = VectorTopo(cls.tmpname)
        cls.vect.open('r')
        cls.vect.close()

    @classmethod
    def tearDownClass(cls):
        if cls.vect.is_open():
            cls.vect.close()

        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='vector',
                      name=cls.tmpname)

    def test_getitem_slice(self):
        """Test that getitem handle correctly the slice starting from 1"""
        vcoords = ((10.0, 6.0), (12.0, 6.0))
        with VectorTopo(self.tmpname, mode="r") as vect:
            coords = tuple([pnt.coords() for pnt in vect[:3]])
            self.assertTupleEqual(vcoords, coords)
            coords = tuple([pnt.coords() for pnt in vect[1:3]])
            self.assertTupleEqual(vcoords, coords)
            self.vect.close()

    def test_viter(self):
        """Test that getitem handle correctly the slice starting from 1"""

        with VectorTopo(self.tmpname, mode="r") as vect:
            for name in ["points", "lines", "areas", "islands", "nodes"]:
                count = 0
                for feature in vect.viter(name):
                    count += 1
                self.assertEqual(count, vect.number_of(name))
                
            self.vect.close()

    def test_getitem_raise(self):
        """Test that getitem raise a value error if the key is not
        an integer or a slice"""
        with VectorTopo(self.tmpname, mode="r") as vect:
            with self.assertRaises(ValueError):
                vect['value']

            self.vect.close()

if __name__ == '__main__':
    test()
