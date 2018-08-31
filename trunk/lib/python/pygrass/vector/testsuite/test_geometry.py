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

from grass.pygrass.vector import Vector, VectorTopo
from grass.pygrass.vector.geometry import Point, Line, Node
from grass.pygrass.vector.geometry import Area, Boundary, Centroid
from grass.pygrass.vector.basic import Bbox

class PointTestCase(TestCase):

    def test_empty_init(self):
        """Test Point()"""
        point = Point()
        self.assertEqual(point.gtype, libvect.GV_POINT)
        self.assertEqual(point.x, 0)
        self.assertEqual(point.y, 0)
        self.assertIsNone(point.z)
        self.assertTrue(point.is2D)

    def test_init_3d(self):
        """Test 3D Point(1, 2, 3)"""
        point = Point(1, 2, 3)
        self.assertEqual(point.x, 1)
        self.assertEqual(point.y, 2)
        self.assertEqual(point.z, 3)
        self.assertFalse(point.is2D)

    def test_switch_2D_3D_2D(self):
        """Test switch between: 2D => 3D => 2D"""
        point = Point()
        self.assertIsNone(point.z)
        self.assertTrue(point.is2D)
        point.z = 1
        self.assertFalse(point.is2D)
        point.z = None
        self.assertTrue(point.is2D, True)

    def test_coords(self):
        """Test coords method"""
        self.assertEqual(Point(1, 2).coords(), (1, 2))
        self.assertEqual(Point(1, 2, 3).coords(), (1, 2, 3))

    def test_to_wkt_p(self):
        """Test coords method"""
        self.assertEqual(Point(1, 2).to_wkt_p(), 'POINT(1.000000 2.000000)')
        self.assertEqual(Point(1, 2, 3).to_wkt_p(),
                         'POINT(1.000000 2.000000 3.000000)')

    def test_to_wkt(self):
        """Test coords method"""
        self.assertEqual(Point(1, 2).to_wkt(), 'POINT (1.0000000000000000 2.0000000000000000)')
        self.assertEqual(Point(1, 2, 3).to_wkt(),
                         'POINT Z (1.0000000000000000 2.0000000000000000 3.0000000000000000)')

    def test_to_wkb(self):
        """Test to_wkb method"""
        self.assertEqual(len(Point(1, 2).to_wkb()), 21)

    def test_distance(self):
        """Test distance method"""
        point0 = Point(0, 0, 0)
        point1 = Point(1, 0)
        self.assertEqual(point0.distance(point1), 1.0)
        point1.z = 1
        self.assertAlmostEqual(point0.distance(point1), np.sqrt(2.))

    def test_eq(self):
        """Test __eq__"""
        point0 = Point(0, 0)
        point1 = Point(1, 0)
        self.assertFalse(point0 == point1)
        self.assertFalse(point0 == (1, 0))
        self.assertTrue(point0 == point0)
        self.assertTrue(point0 == (0, 0))

    def test_repr(self):
        """Test __eq__"""
        self.assertEqual(repr(Point(1, 2)), 'Point(1.000000, 2.000000)')
        self.assertEqual(repr(Point(1, 2, 3)),
                         'Point(1.000000, 2.000000, 3.000000)')

    @unittest.skip("Not implemented yet.")
    def test_buffer(self):
        """Test buffer method"""
        # TODO: verify if the buffer depends from the mapset's projection
        pass


class LineTestCase(TestCase):

    tmpname = "LineTestCase_map"

    @classmethod
    def setUpClass(cls):

        from grass.pygrass import utils
        utils.create_test_vector_map(cls.tmpname)

        cls.vect = None
        cls.vect = VectorTopo(cls.tmpname)
        cls.vect.open('r')
        cls.c_mapinfo = cls.vect.c_mapinfo

    @classmethod
    def tearDownClass(cls):
        if cls.vect is not None:
            cls.vect.close()
            cls.c_mapinfo = None

        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='vector', 
                      name=cls.tmpname)

    def test_len(self):
        """Test __len__ magic method"""
        self.assertEqual(len(Line()), 0)
        self.assertEqual(len(Line([(0, 0), (1, 1)])), 2)

    @unittest.skipIf(sys.version_info[:2] < (2, 7), "Require Python >= 2.7")
    def test_getitem(self):
        """Test __getitem__ magic method"""
        line = Line([(0, 0), (1, 1), (2, 2), (3, 3), (4, 4)])
        self.assertTupleEqual(line[0].coords(), (0, 0))
        self.assertTupleEqual(line[1].coords(), (1, 1))
        self.assertTupleEqual(line[-2].coords(), (3, 3))
        self.assertTupleEqual(line[-1].coords(), (4, 4))
        self.assertListEqual([p.coords() for p in line[:2]], [(0, 0), (1, 1)])
        self.assertListEqual([p.coords() for p in line[::2]],
                             [(0, 0), (2, 2), (4, 4)])
        with self.assertRaises(IndexError):
            line[5]

    @unittest.skipIf(sys.version_info[:2] < (2, 7), "Require Python >= 2.7")
    def test_setitem(self):
        """Test __setitem__ magic method"""
        line = Line([(0, 0), (1, 1)])
        self.assertTupleEqual(line[0].coords(), (0., 0.))
        line[0] = (10, 10)
        self.assertTupleEqual(line[0].coords(), (10., 10.))

    @unittest.skipIf(sys.version_info[:2] < (2, 7), "Require Python >= 2.7")
    def test_get_pnt(self):
        """Test get_pnt method"""
        line = Line([(0, 0), (1, 1)])
        with self.assertRaises(ValueError):
            line.point_on_line(5)
        vals = (0.7071067811865475, 0.7071067811865475)
        self.assertTupleEqual(line.point_on_line(1).coords(), vals)

    def test_to_wkt(self):
        """Test to_wkt method"""
        string = 'LINESTRING (0.0000000000000000 0.0000000000000000, 1.0000000000000000 1.0000000000000000)'
        self.assertEqual(Line([(0, 0), (1, 1)]).to_wkt(), string)

    def test_to_wkb(self):
        """Test to_wkb method"""
        self.assertEqual(len(Line([(0, 0), (1, 1)]).to_wkb()), 41)

    def test_bbox(self):
        """Test bbox method"""
        line = Line([(0, 10), (0, 11), (1, 11), (1, 10)])
        bbox = line.bbox()
        self.assertEqual(11, bbox.north)
        self.assertEqual(10, bbox.south)
        self.assertEqual(1, bbox.east)
        self.assertEqual(0, bbox.west)

    def test_nodes(self):
        """Test nodes method"""
        def nodes2tuple(nodes):
            """Convert an iterable of nodes to a tuple of nodes id"""
            return tuple(n.id for n in nodes)

        with VectorTopo("LineTestCase_map", mode='r') as vect:
            self.assertTupleEqual((1, 2), nodes2tuple(vect[4].nodes()))
            self.assertTupleEqual((3, 4), nodes2tuple(vect[5].nodes()))
            self.assertTupleEqual((5, 6), nodes2tuple(vect[6].nodes()))

class NodeTestCase(TestCase):

    tmpname = "NodeTestCase_map"

    @classmethod
    def setUpClass(cls):

        # Tests are based on a stream network
        from grass.pygrass import utils
        utils.create_test_stream_network_map(cls.tmpname)

        cls.vect = None
        cls.vect = VectorTopo(cls.tmpname)
        cls.vect.open('r')
        cls.c_mapinfo = cls.vect.c_mapinfo

    @classmethod
    def tearDownClass(cls):
        if cls.vect is not None:
            cls.vect.close()
            cls.c_mapinfo = None

        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='vector', 
                      name=cls.tmpname)

    def test_init(self):
        """Test Node __init__"""
        node = Node(v_id=4, c_mapinfo=self.c_mapinfo)
        self.assertEqual(4, node.id)
        self.assertTrue(node.is2D)
        self.assertEqual(5, node.nlines)

    def test_coords(self):
        """Test Node coordinates"""
        node = Node(v_id=4, c_mapinfo=self.c_mapinfo)
        self.assertTupleEqual((1.0, 0.0), node.coords())

    def test_ilines(self):
        """Test Node neighbors"""
        node = Node(v_id=4, c_mapinfo=self.c_mapinfo)
        self.assertTupleEqual((6, -4, 7, -3, -5), tuple(node.ilines()))
        self.assertTupleEqual((-4, -3, -5), tuple(node.ilines(only_in=True)))
        node = Node(v_id=4, c_mapinfo=self.c_mapinfo)
        self.assertTupleEqual((6, 7), tuple(node.ilines(only_out=True)))

    def test_angles(self):
        """Test Node angles"""
        node = Node(v_id=4, c_mapinfo=self.c_mapinfo)
        angles = (-1.5707963705062866, 0.7853981852531433,
                   1.2793395519256592, 1.8622530698776245,
                   2.356194496154785)
        self.assertTupleEqual(angles, tuple(node.angles()))

class AreaTestCase(TestCase):

    tmpname = "AreaTestCase_map"

    @classmethod
    def setUpClass(cls):

        # Tests are based on a stream network
        from grass.pygrass import utils
        utils.create_test_vector_map(cls.tmpname)

        cls.vect = None
        cls.vect = VectorTopo(cls.tmpname)
        cls.vect.open('r')
        cls.c_mapinfo = cls.vect.c_mapinfo

    @classmethod
    def tearDownClass(cls):
        if cls.vect is not None:
            cls.vect.close()
            cls.c_mapinfo = None

        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='vector', 
                      name=cls.tmpname)

    def test_init(self):
        """Test area __init__ and basic functions"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        self.assertEqual(1, area.id)
        self.assertTrue(area.is2D)
        self.assertTrue(area.alive())
        self.assertEqual(area.area(), 12.0)

    def test_to_wkt(self):
        """Test to_wkt method"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        # Outer and inner ring!!
        string = "POLYGON ((0.0000000000000000 0.0000000000000000, "\
                           "0.0000000000000000 4.0000000000000000, "\
                           "0.0000000000000000 4.0000000000000000, "\
                           "4.0000000000000000 4.0000000000000000, "\
                           "4.0000000000000000 4.0000000000000000, "\
                           "4.0000000000000000 0.0000000000000000, "\
                           "4.0000000000000000 0.0000000000000000, "\
                           "0.0000000000000000 0.0000000000000000), "\
                           "(1.0000000000000000 1.0000000000000000, "\
                           "3.0000000000000000 1.0000000000000000, "\
                           "3.0000000000000000 3.0000000000000000, "\
                           "1.0000000000000000 3.0000000000000000, "\
                           "1.0000000000000000 1.0000000000000000))"
        self.assertEqual(area.to_wkt(), string)

    def test_to_wkb(self):
        """Test to_wkt method"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        self.assertEqual(len(area.to_wkb()), 225)

    def test_contains_point(self):
        """Test contain_point method"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        p = Point(0.5, 0.5)
        bbox = Bbox(4.0, 0.0, 4.0, 0.0)
        self.assertTrue(area.contains_point(p, bbox))
        self.assertTrue(area.contains_point(p))

    def test_bbox(self):
        """Test contain_point method"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        self.assertEqual(str(area.bbox()), "Bbox(4.0, 0.0, 4.0, 0.0)")

    def test_centroid(self):
        """Test centroid access"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        centroid = area.centroid()

        self.assertEqual(centroid.id, 18)
        self.assertEqual(centroid.area_id, 1)
        self.assertEqual(centroid.to_wkt(), 'POINT (3.5000000000000000 3.5000000000000000)')

    def test_boundaries_1(self):
        """Test boundary access"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        boundaries = area.boundaries()
        self.assertEqual(len(boundaries), 4)

        string_list = []
        string_list.append("LINESTRING (0.0000000000000000 0.0000000000000000, 0.0000000000000000 4.0000000000000000)")
        string_list.append("LINESTRING (0.0000000000000000 4.0000000000000000, 4.0000000000000000 4.0000000000000000)")
        string_list.append("LINESTRING (4.0000000000000000 4.0000000000000000, 4.0000000000000000 0.0000000000000000)")
        string_list.append("LINESTRING (4.0000000000000000 0.0000000000000000, 0.0000000000000000 0.0000000000000000)")

        for boundary, i in zip(boundaries, range(4)):
            self.assertEqual(len(boundary.to_wkb()), 41)
            self.assertEqual(boundary.to_wkt(), string_list[i])

    def test_boundaries_2(self):
        """Test boundary access"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        boundaries = area.boundaries()
        boundary = boundaries[2]
        boundary.read_area_ids()
        self.assertEqual(boundary.left_area_id, 2)
        self.assertEqual(boundary.right_area_id, 1)

        self.assertEqual(boundary.left_centroid().to_wkt(), 'POINT (5.5000000000000000 3.5000000000000000)')
        self.assertEqual(boundary.right_centroid().to_wkt(), 'POINT (3.5000000000000000 3.5000000000000000)')

    def test_isles_1(self):
        """Test centroid access"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        self.assertEqual(area.num_isles(), 1)

        isles = area.isles()
        isle = isles[0]

        self.assertEqual(isle.area(), 4.0)
        self.assertEqual(isle.points().to_wkt(), "LINESTRING (1.0000000000000000 1.0000000000000000, "\
                                                             "3.0000000000000000 1.0000000000000000, "\
                                                             "3.0000000000000000 3.0000000000000000, "\
                                                             "1.0000000000000000 3.0000000000000000, "\
                                                             "1.0000000000000000 1.0000000000000000)")
    def test_isles_2(self):
        """Test centroid access"""
        area = Area(v_id=1, c_mapinfo=self.c_mapinfo)
        self.assertEqual(area.num_isles(), 1)

        isles = area.isles()
        isle = isles[0]
        self.assertEqual(isle.area_id(), 1)
        self.assertTrue(isle.alive())

        self.assertEqual(str(isle.bbox()), "Bbox(3.0, 1.0, 3.0, 1.0)")


if __name__ == '__main__':
    test()
