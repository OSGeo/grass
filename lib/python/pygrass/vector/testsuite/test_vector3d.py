# -*- coding: utf-8 -*-
"""
Created on Wed Jun 18 17:21:42 2014

@author: pietro
"""
import numpy as np

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import run_command

from grass.pygrass.vector import VectorTopo
from grass.pygrass.vector.geometry import Point
from grass.pygrass.gis.region import Region
from grass.pygrass.utils import get_mapset_vector


def generate_coordinates(number, bbox=None, with_z=False):
    """Return 2 or 3 random arrays of coordinates"""
    bbox = Region() if bbox is None else bbox
    x = bbox.south + (bbox.north - bbox.south) * np.random.random(number)
    y = bbox.west + (bbox.east - bbox.west) * np.random.random(number)
    if with_z:
        z = np.random.random(number) * 1000
        return x, y, z
    return x, y


class VectorTopo3DTestCase(TestCase):

    npoints = 10
    tmpname = "tmp_vect3d"

    @classmethod
    def setUpClass(cls):
        """Generate a number (NPOINTS) of random points"""
        cls.x, cls.y, cls.z = generate_coordinates(cls.npoints, with_z=True)

    def writing_points(self):
        """Write the generated random points to a vector map"""
        with VectorTopo(self.tmpname, mode="w", with_z=True) as vect:
            for x, y, z in zip(self.x, self.y, self.z):
                vect.write(Point(x, y, z))

    def reading_points(self):
        """Read the generated random points from a vector map"""
        with VectorTopo(self.tmpname, mode="r") as vect:
            # reading the generated vector points map
            arr = np.array([(p.x, p.y, p.z) for p in vect])
            # verify the correspondence
            for i, coords in enumerate((self.x, self.y, self.z)):
                np.testing.assert_almost_equal(arr.T[i], coords)

    def test_writing_reading_points(self):
        self.writing_points()
        self.reading_points()

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='vector', 
                      name=cls.tmpname)


if __name__ == '__main__':
    test()
