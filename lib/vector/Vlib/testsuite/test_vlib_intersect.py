"""
TEST:      intersect.c & intersect2.c

AUTHOR(S): Maris Nartiss

PURPOSE:   Test functions related to line intersection

COPYRIGHT: (C) 2022 Maris Nartiss, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import grass.lib.vector as libvect
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestLineIntersect(TestCase):
    """Test functions related to line intersection"""

    @classmethod
    def setUpClass(cls):
        """Create lines for intersecting"""
        cls.c_lp_snake = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_snake, 1, 1, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 1, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 2, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 2, 1, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 3, 1, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 3, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 4, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 4, 1, 0)
        cls.c_lp_ssnake = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_ssnake, 1.5, 2, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 1.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 2.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 2.5, 2, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 3.5, 2, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 3.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 4.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 4.5, 2, 0)
        cls.c_lp_scross = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_scross, 0, 2, 0)
        libvect.Vect_append_point(cls.c_lp_scross, 1.5, 2, 0)
        cls.c_lp_mcross = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_mcross, 0, 2, 0)
        libvect.Vect_append_point(cls.c_lp_mcross, 5, 2, 0)
        cls.c_lp_fmiss = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_fmiss, 10, 10, 0)
        libvect.Vect_append_point(cls.c_lp_fmiss, 20, 20, 0)
        cls.c_lp_nmiss = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_nmiss, 2.5, 1.5, 0)
        libvect.Vect_append_point(cls.c_lp_nmiss, 2.5, 2.5, 0)
        cls.c_lp_cont1 = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_cont1, 6, 1, 0)
        libvect.Vect_append_point(cls.c_lp_cont1, 8, 1, 0)
        cls.c_lp_cont2 = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_cont2, 6, 2, 0)
        libvect.Vect_append_point(cls.c_lp_cont2, 8, 2, 0)
        cls.c_lp_cont3 = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_cont3, 8, 2, 0)
        libvect.Vect_append_point(cls.c_lp_cont3, 9, 2, 0)

    @classmethod
    def tearDownClass(cls):
        """Free memory just in case"""
        libvect.Vect_destroy_line_struct(cls.c_lp_snake)
        libvect.Vect_destroy_line_struct(cls.c_lp_ssnake)
        libvect.Vect_destroy_line_struct(cls.c_lp_scross)
        libvect.Vect_destroy_line_struct(cls.c_lp_mcross)
        libvect.Vect_destroy_line_struct(cls.c_lp_fmiss)
        libvect.Vect_destroy_line_struct(cls.c_lp_nmiss)
        libvect.Vect_destroy_line_struct(cls.c_lp_cont1)
        libvect.Vect_destroy_line_struct(cls.c_lp_cont2)
        libvect.Vect_destroy_line_struct(cls.c_lp_cont3)

    def testNoFarGetIntersections(self):
        """Both lines are far away"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_snake, self.c_lp_fmiss, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)

    def testNoNearGetIntersections(self):
        """Second line is inside first bbox but do not cross"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_snake, self.c_lp_nmiss, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)

    def testSingleGetIntersections(self):
        """Both lines have a single intersection point"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_snake, self.c_lp_scross, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 1)

    def testColinearIntersections(self):
        """Line overlap or continuation"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_cont1, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_cont2, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_scross, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_cont2, self.c_lp_cont3, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 1)

    def testMultiGetIntersections(self):
        """Both lines intersect at multiple points"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_snake, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 4)
        libvect.Vect_reset_line(c_lp_cps)
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_snake, self.c_lp_ssnake, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 3)
        # Order of intersecting lines should not matter
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_mcross, self.c_lp_snake, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 4)
        libvect.Vect_reset_line(c_lp_cps)
        ret = libvect.Vect_line_get_intersections(
            self.c_lp_ssnake, self.c_lp_snake, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 3)

    def testNoFarCheckIntersection(self):
        """Both lines are far away"""
        ret = libvect.Vect_line_check_intersection(self.c_lp_snake, self.c_lp_fmiss, 0)
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_check_intersection(self.c_lp_snake, self.c_lp_fmiss, 0)
        self.assertEqual(ret, 0)

    def testNoNearCheckIntersection(self):
        """Second line is inside first bbox but do not cross"""
        ret = libvect.Vect_line_check_intersection(self.c_lp_snake, self.c_lp_nmiss, 0)
        self.assertEqual(ret, 0)

    def testSingleCheckIntersection(self):
        """Both lines have a single intersection point"""
        ret = libvect.Vect_line_check_intersection(self.c_lp_snake, self.c_lp_scross, 0)
        self.assertEqual(ret, 1)

    def testColinearCheckIntersections(self):
        """Line overlap or continuation"""
        ret = libvect.Vect_line_check_intersection(self.c_lp_cont1, self.c_lp_mcross, 0)
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_check_intersection(self.c_lp_cont2, self.c_lp_mcross, 0)
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_check_intersection(
            self.c_lp_scross, self.c_lp_mcross, 0
        )
        self.assertEqual(ret, 1)
        ret = libvect.Vect_line_check_intersection(self.c_lp_cont2, self.c_lp_cont3, 0)
        self.assertEqual(ret, 1)

    def testMultiCheckIntersections(self):
        """Both lines intersect at multiple points"""
        ret = libvect.Vect_line_check_intersection(self.c_lp_snake, self.c_lp_mcross, 0)
        self.assertEqual(ret, 1)
        ret = libvect.Vect_line_check_intersection(self.c_lp_snake, self.c_lp_ssnake, 0)
        self.assertEqual(ret, 1)


class TestLineIntersect2(TestCase):
    """Test functions related to line intersection"""

    @classmethod
    def setUpClass(cls):
        """Create lines for intersecting"""
        cls.c_lp_snake = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_snake, 1, 1, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 1, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 2, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 2, 1, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 3, 1, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 3, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 4, 3, 0)
        libvect.Vect_append_point(cls.c_lp_snake, 4, 1, 0)
        cls.c_lp_ssnake = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_ssnake, 1.5, 2, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 1.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 2.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 2.5, 2, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 3.5, 2, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 3.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 4.5, 4, 0)
        libvect.Vect_append_point(cls.c_lp_ssnake, 4.5, 2, 0)
        cls.c_lp_scross = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_scross, 0, 2, 0)
        libvect.Vect_append_point(cls.c_lp_scross, 1.5, 2, 0)
        cls.c_lp_mcross = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_mcross, 0, 2, 0)
        libvect.Vect_append_point(cls.c_lp_mcross, 5, 2, 0)
        cls.c_lp_fmiss = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_fmiss, 10, 10, 0)
        libvect.Vect_append_point(cls.c_lp_fmiss, 20, 20, 0)
        cls.c_lp_nmiss = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_nmiss, 2.5, 1.5, 0)
        libvect.Vect_append_point(cls.c_lp_nmiss, 2.5, 2.5, 0)
        cls.c_lp_cont1 = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_cont1, 6, 1, 0)
        libvect.Vect_append_point(cls.c_lp_cont1, 8, 1, 0)
        cls.c_lp_cont2 = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_cont2, 6, 2, 0)
        libvect.Vect_append_point(cls.c_lp_cont2, 8, 2, 0)
        cls.c_lp_cont3 = libvect.Vect_new_line_struct()
        libvect.Vect_append_point(cls.c_lp_cont3, 8, 2, 0)
        libvect.Vect_append_point(cls.c_lp_cont3, 9, 2, 0)

    @classmethod
    def tearDownClass(cls):
        """Free memory just in case"""
        libvect.Vect_destroy_line_struct(cls.c_lp_snake)
        libvect.Vect_destroy_line_struct(cls.c_lp_ssnake)
        libvect.Vect_destroy_line_struct(cls.c_lp_scross)
        libvect.Vect_destroy_line_struct(cls.c_lp_mcross)
        libvect.Vect_destroy_line_struct(cls.c_lp_fmiss)
        libvect.Vect_destroy_line_struct(cls.c_lp_nmiss)
        libvect.Vect_destroy_line_struct(cls.c_lp_cont1)
        libvect.Vect_destroy_line_struct(cls.c_lp_cont2)
        libvect.Vect_destroy_line_struct(cls.c_lp_cont3)

    def testNoFarGetIntersections2(self):
        """Both lines are far away"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_snake, self.c_lp_fmiss, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)

    def testNoNearGetIntersections2(self):
        """Second line is inside first bbox but do not cross"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_snake, self.c_lp_nmiss, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)

    def testSingleGetIntersections2(self):
        """Both lines have a single intersection point"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_snake, self.c_lp_scross, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 1)

    def testColinearIntersections2(self):
        """Line overlap or continuation"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_cont1, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_cont2, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_scross, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_cont2, self.c_lp_cont3, c_lp_cps, 0
        )
        self.assertEqual(ret, 2)
        self.assertEqual(c_lp_cps.contents.n_points, 1)

    def testMultiGetIntersections2(self):
        """Both lines intersect at multiple points"""
        c_lp_cps = libvect.Vect_new_line_struct()
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_snake, self.c_lp_mcross, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 4)
        libvect.Vect_reset_line(c_lp_cps)
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_snake, self.c_lp_ssnake, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 3)
        # Order of intersecting lines should not matter
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_mcross, self.c_lp_snake, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 4)
        libvect.Vect_reset_line(c_lp_cps)
        ret = libvect.Vect_line_get_intersections2(
            self.c_lp_ssnake, self.c_lp_snake, c_lp_cps, 0
        )
        self.assertEqual(ret, 1)
        self.assertEqual(c_lp_cps.contents.n_points, 3)

    def testNoFarCheckIntersection2(self):
        """Both lines are far away"""
        ret = libvect.Vect_line_check_intersection2(self.c_lp_snake, self.c_lp_fmiss, 0)
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_check_intersection2(self.c_lp_snake, self.c_lp_fmiss, 0)
        self.assertEqual(ret, 0)

    def testNoNearCheckIntersection2(self):
        """Second line is inside first bbox but do not cross"""
        ret = libvect.Vect_line_check_intersection2(self.c_lp_snake, self.c_lp_nmiss, 0)
        self.assertEqual(ret, 0)

    def testSingleCheckIntersection2(self):
        """Both lines have a single intersection point"""
        ret = libvect.Vect_line_check_intersection2(
            self.c_lp_snake, self.c_lp_scross, 0
        )
        self.assertEqual(ret, 1)

    def testColinearCheckIntersections2(self):
        """Line overlap or continuation"""
        ret = libvect.Vect_line_check_intersection2(
            self.c_lp_cont1, self.c_lp_mcross, 0
        )
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_check_intersection2(
            self.c_lp_cont2, self.c_lp_mcross, 0
        )
        self.assertEqual(ret, 0)
        ret = libvect.Vect_line_check_intersection2(
            self.c_lp_scross, self.c_lp_mcross, 0
        )
        self.assertEqual(ret, 1)
        ret = libvect.Vect_line_check_intersection2(self.c_lp_cont2, self.c_lp_cont3, 0)
        self.assertEqual(ret, 2)

    def testMultiCheckIntersections2(self):
        """Both lines intersect at multiple points"""
        ret = libvect.Vect_line_check_intersection2(
            self.c_lp_snake, self.c_lp_mcross, 0
        )
        self.assertEqual(ret, 1)
        ret = libvect.Vect_line_check_intersection2(
            self.c_lp_snake, self.c_lp_ssnake, 0
        )
        self.assertEqual(ret, 1)


if __name__ == "__main__":
    test()
