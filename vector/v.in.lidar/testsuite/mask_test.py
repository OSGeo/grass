"""
Name:      decimation_test
Purpose:   v.in.lidar decimation test

Author:    Vaclav Petras
Copyright: (C) 2015 by Vaclav Petras and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

POINTS = """\
17.46938776,18.67346939,1
20.93877551,17.44897959,2
18.89795918,14.18367347,3
15.91836735,10.67346939,4
21.26530612,11.04081633,5
22.24489796,13.89795918,6
23.79591837,17.12244898,7
17.2244898,16.34693878,8
17.14285714,14.10204082,9
19.87755102,11.81632653,10
18.48979592,11.48979592,11
21.26530612,15.73469388,12
21.18367347,19.32653061,13
23.91836735,18.83673469,14
23.51020408,13.65306122,15
23.55102041,11.32653061,16
18.41009273,14.51618034,17
22.13996161,17.2278263,18
21.41013052,11.05432488,19
"""

# the point with cat 4 is outside the bbox of the areas vector map
# which is what we need for the tests

AREAS = """\
ORGANIZATION:
DIGIT DATE:
DIGIT NAME:   vpetras
MAP NAME:
MAP DATE:     Tue Dec 22 17:22:54 2015
MAP SCALE:    1
OTHER INFO:
ZONE:         0
MAP THRESH:   0.000000
VERTI:
B  6
 16.89795918  17.28571429
 19.75510204  15.12244898
 21.34693878  12.87755102
 17.3877551   12.67346939
 16           15.24489796
 16.89795918  17.28571429
C  1 1
 18.41009273  14.51618034
 1     17
B  6
 20.20408163  19.57142857
 21.14285714  14.63265306
 23.75510204  15.08163265
 24.36734694  17.57142857
 22.20408163  19.73469388
 20.20408163  19.57142857
C  1 1
 22.13996161  17.2278263
 1     18
B  5
 20.89795918  11.57142857
 22.40816327  11.04081633
 21.02040816  10.51020408
 20.57142857  11.24489796
 20.89795918  11.57142857
C  1 1
 21.41013052  11.05432488
 1     19
"""


import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class VectorMaskTest(TestCase):
    """Test case for watershed module

    This tests expects v.random and v.out.lidar to work properly.
    """

    # Setup variables to be used for outputs
    points = 'vinlidar_points'
    areas = 'vinlidar_areas'
    las_file = 'vinlidar_mask_points.las'
    imported_points = 'vinlidar_imported_points'

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule('g.region', n=20, s=10, e=25, w=15, res=1)
        cls.runModule('v.in.ascii', input='-', output=cls.points,
                      separator='comma', format='point', stdin_=POINTS)
        cls.runModule('v.in.ascii', input='-', output=cls.areas,
                      format='standard', stdin_=AREAS)
        cls.runModule('v.out.lidar', input=cls.points,
            output=cls.las_file)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule('g.remove', flags='f', type='vector',
            name=(cls.points, cls.areas))
        if os.path.isfile(cls.las_file):
            os.remove(cls.las_file)
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created by the import

        This is executed after each test run.
        """
        self.runModule('g.remove', flags='f', type='vector',
            name=self.imported_points)

    def test_no_mask(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt')
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=19))

    def test_mask(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='bt',
            mask=self.areas)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=11))

    def test_inverted_mask(self):
        """Test to see if the standard outputs are created"""
        self.assertModule('v.in.lidar', input=self.las_file,
            output=self.imported_points, flags='btu',
            mask=self.areas)
        self.assertVectorExists(self.imported_points)
        self.assertVectorFitsTopoInfo(
            vector=self.imported_points,
            reference=dict(points=8))


if __name__ == '__main__':
    test()
