"""
Name:      filter_test
Purpose:   v.in.pdal test if various filters and selections

Author:    Vaclav Petras
Copyright: (C) 2015 by Vaclav Petras and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import unittest
from grass.script import shutil_which

POINTS = """\
17.46938776,18.67346939,143,1,1,2
20.93877551,17.44897959,125,1,1,2
18.89795918,14.18367347,130,1,1,3
15.91836735,10.67346939,126,1,1,3
21.26530612,11.04081633,128,1,2,3
22.24489796,13.89795918,123,2,2,3
23.79591837,17.12244898,151,1,2,3
17.2244898,16.34693878,124,2,2,4
17.14285714,14.10204082,134,1,3,4
19.87755102,11.81632653,146,2,3,4
18.48979592,11.48979592,140.6,2,3,4
21.26530612,15.73469388,147,3,3,5
21.18367347,19.32653061,138,1,3,5
23.91836735,18.83673469,144,2,3,5
23.51020408,13.65306122,143,3,3,5
23.55102041,11.32653061,123,1,4,5
18.41009273,14.51618034,140.4,2,4,5
22.13996161,17.2278263,147,3,4,5
21.41013052,11.05432488,132,4,4,5
"""


class FilterTest(TestCase):
    """Test case for filter and selection options

    This tests expects v.random and v.out.lidar to work properly.
    """

    # Setup variables to be used for outputs
    vector_points = 'vinlidar_filters_original'
    imported_points = 'vinlidar_filters_imported'
    las_file = 'vinlidar_filters_points.las'
    npoints = 300

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule('g.region', n=20, s=10, e=25, w=15, res=1)
        cls.runModule('v.in.ascii', input='-', stdin_=POINTS,
                      flags='z', z=3, cat=0, separator='comma',
                      output=cls.vector_points,
                      columns="x double precision, y double precision,"
                              " z double precision, return_n integer,"
                              " n_returns integer, class_n integer")
        cls.runModule('v.out.lidar',
                      input=cls.vector_points, layer=1,
                      output=cls.las_file,
                      return_column='return_n',
                      n_returns_column='n_returns',
                      class_column='class_n')

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule('g.remove', flags='f', type='vector',
            name=cls.vector_points)
        if os.path.isfile(cls.las_file):
            os.remove(cls.las_file)
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created by the import

        This is executed after each test run.
        """
        self.runModule('g.remove', flags='f', type='vector',
            name=self.imported_points)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_no_filter(self):
        """Test to see if the standard outputs are created

        This shows if the inpute data are as expected.
        """
        self.assertModule('v.in.pdal', input=self.las_file,
            output=self.imported_points)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=19))

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def return_filter(self, name, npoints):
        """Mid return filter test"""
        self.assertModule('v.in.pdal', input=self.las_file,
            output=self.imported_points,
            return_filter=name)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=npoints))

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_first_return_filter(self):
        """First return filter test"""
        self.return_filter('first', 9)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_mid_return_filter(self):
        """Mid return filter test"""
        self.return_filter('mid', 5)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_last_return_filter(self):
        """Last return filter test"""
        self.return_filter('last', 5)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def class_filter(self, class_n, npoints):
        """Actual code for testing class filter"""
        self.assertModule('v.in.pdal', input=self.las_file,
            output=self.imported_points,
            class_filter=class_n)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=npoints))

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_class_2_filter(self):
        """Test to filter classes"""
        self.class_filter(2, 2)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_class_3_filter(self):
        """Test to filter classes"""
        self.class_filter(3, 5)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_class_4_filter(self):
        """Test to filter classes"""
        self.class_filter(4, 4)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_class_5_filter(self):
        """Test to filter classes"""
        self.class_filter(5, 8)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def return_and_class_filter(self, return_name, class_n, npoints):
        """Return and class filter combined test code"""
        self.assertModule('v.in.pdal', input=self.las_file,
            output=self.imported_points,
            return_filter=return_name, class_filter=class_n)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=npoints))

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_first_return_and_class_filter(self):
        """Combined test for return and class"""
        self.return_and_class_filter('first', 2, 2)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_last_return_and_class_filter(self):
        """Combined test for return and class"""
        self.return_and_class_filter('last', 5, 3)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def zrange_filter(self, zrange, npoints):
        """Actual code for zrange option test"""
        self.assertModule('v.in.pdal', input=self.las_file,
            output=self.imported_points,
            zrange=zrange)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=npoints))

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_zrange_filter(self):
        """Test zrange option"""
        self.zrange_filter((130.1, 139.9), 3)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_non_int_zrange_filter(self):
        """Test zrange option with float number

        One test point has z right under and one other right above the min.
        """
        self.zrange_filter((140.5, 900), 8)

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_zrange_and_class_filter(self):
        """zrange and class_filter option combined test"""
        self.assertModule('v.in.pdal', input=self.las_file,
            output=self.imported_points,
            zrange=(141, 900), class_filter=5)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=4))

    @unittest.skipIf(shutil_which('v.in.pdal') is None, "Cannot find v.in.pdal")
    def test_zrange_and_return_filter(self):
        """zrange and class_filter option combined test"""
        self.assertModule('v.in.pdal', input=self.las_file,
            output=self.imported_points,
            zrange=(141, 900), return_filter='last')
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=2))


if __name__ == '__main__':
    test()
