"""
TEST:      box.c

AUTHOR(S): Vaclav Petras

PURPOSE:   Test functions related to bounding box

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import ctypes
import grass.lib.vector as libvect

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestPointInBoundingBox(TestCase):
    """Test functions related to point in bounding box"""

    def setUp(self):
        """Create bbox object"""
        self.c_bbox = ctypes.pointer(libvect.bound_box())
        # x range
        self.c_bbox.contents.E = 220
        self.c_bbox.contents.W = 215
        # y range
        self.c_bbox.contents.N = 135
        self.c_bbox.contents.S = 125
        # z range
        self.c_bbox.contents.T = 340
        self.c_bbox.contents.B = 330

    def test_bbox_3d_in(self):
        """Check Vect_point_in_box() function with 3D points inside bbox"""
        self.check_point_in_3d(217, 130, 335)
        self.check_point_in_3d(219.999999, 125.000001, 339.999999)

    def test_bbox_3d_out(self):
        """Check Vect_point_in_box() function with 3D points outside bbox"""
        self.check_point_out_3d(100, 100, 100)
        self.check_point_out_3d(500, 593, 900)
        self.check_point_out_3d(-220, 130, 335)
        self.check_point_out_3d(220, -130, 335)
        self.check_point_out_3d(220, 130, -335)

    def check_point_in_3d(self, x, y, z):
        """Wraps Vect_point_in_box() with assert and a message"""
        self.assertTrue(libvect.Vect_point_in_box(x, y, z, self.c_bbox),
                        msg="Point should be inside the bbox")

    def check_point_out_3d(self, x, y, z):
        """Wraps Vect_point_in_box() with assert and a message"""
        self.assertFalse(libvect.Vect_point_in_box(x, y, z, self.c_bbox),
                         msg="Point should be outside the bbox")

    def test_bbox_2d_in(self):
        """Check Vect_point_in_box_2d() function with 2D points inside bbox"""
        self.check_point_in_2d(217, 130)
        self.check_point_in_2d(219.999999, 125.000001)

    def test_bbox_2d_out(self):
        """Check Vect_point_in_box_2d() function with 2D points outside bbox"""
        self.check_point_out_2d(100, 100)
        self.check_point_out_2d(500, 593)
        self.check_point_out_2d(-220, 130)
        self.check_point_out_2d(220, -130)
        self.check_point_out_2d(-220, -130)

    def check_point_in_2d(self, x, y):
        """Wraps Vect_point_in_box_2d() with assert, message and bbox"""
        self.assertTrue(libvect.Vect_point_in_box_2d(x, y, self.c_bbox),
                        msg="Point should be inside the bbox")

    def check_point_out_2d(self, x, y):
        """Wraps Vect_point_in_box_2d() with assert, message and bbox"""
        self.assertFalse(libvect.Vect_point_in_box_2d(x, y, self.c_bbox),
                         msg="Point should be outside the bbox")


if __name__ == '__main__':
    test()
