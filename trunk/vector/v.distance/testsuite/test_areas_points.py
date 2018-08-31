"""
TEST:      v.distance

AUTHOR(S): Vaclav Petras

PURPOSE:   Test v.distance 2D and 3D points with areas

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module

areas = """\
VERTI:
B  6
 11.6         29.55
 12.01        27.01
 15.25        26.68
 16.32        29.42
 13.49        31.06
 11.6         29.55
C  1 1
 13.85        28.69
 1     1
B  10
 18.49        34.1
 21.73        28.85
 28.24        33.28
 26.6         37.17
 23.9         34.92
 22.75        36.92
 18.86        37.17
 14.1         36.19
 14.19        32.54
 18.49        34.1
C  1 1
 21.11        34.03
 1     2
"""

points = """\
13.45|29.96
14.64|27.5
18.04|35.2
21.56|34.22
23.2|32.13
26.48|35.98
14.76|33.44
21.85|29.38
"""

points_3d = """\
13.45|29.96|200
14.64|27.5|250
18.04|35.2|893
21.56|34.22|350
23.2|32.13|296
26.48|35.98|250
14.76|33.44|258
21.85|29.38|257
"""

table_ref = """\
1|South-west area
2|South-west area
3|North-east area
4|North-east area
5|North-east area
6|North-east area
7|North-east area
8|North-east area
"""


class TestPointsAndAreas(TestCase):
    """Test how points get attributes from areas

    Created for #2734 (3D point inside area is classified as outside)
    https://trac.osgeo.org/grass/ticket/2734
    """
    # TODO: replace by unified handing of maps
    to_remove = []
    areas = "test_vdistance_areas"
    points = "test_vdistance_points"
    points_3d = "test_vdistance_points_3d"

    @classmethod
    def setUpClass(cls):
        """Create vector maps (areas with attributes)"""
        cls.runModule('v.in.ascii', input='-', output=cls.areas,
                      format='standard', stdin=areas)
        cls.to_remove.append(cls.areas)
        cls.runModule('v.db.addtable', map=cls.areas,
                      columns="number INTEGER, label VARCHAR(250)")
        # TODO: this should be done in more effective way
        cls.runModule('v.db.update', map=cls.areas, column='label',
                      value="South-west area", where="cat=1")
        cls.runModule('v.db.update', map=cls.areas, column='label',
                      value="North-east area", where="cat=2")

        cls.runModule('v.in.ascii', input='-', output=cls.points,
                      format='point', separator='pipe', flags='t',
                      stdin=points)
        cls.to_remove.append(cls.points)
        cls.runModule('v.db.addtable', map=cls.points,
                      columns="area_label VARCHAR(250)")

        cls.runModule('v.in.ascii', input='-', output=cls.points_3d,
                      format='point', separator='pipe', flags='zt', z=3,
                      stdin=points_3d)
        cls.to_remove.append(cls.points_3d)
        cls.runModule('v.db.addtable', map=cls.points_3d,
                      columns="area_label VARCHAR(250)")

    @classmethod
    def tearDownClass(cls):
        """Remove vector maps"""
        if cls.to_remove:
            cls.runModule('g.remove', flags='f', type='vector',
                          name=','.join(cls.to_remove), verbose=True)

    def test_area_attrs_to_2d_points(self):
        """Check that values are uploaded to 2D points in areas (dmax=0)"""
        # using call_module because PyGRASS doen't accept form_
        call_module('v.distance', from_=self.points, to=self.areas,
                    dmax=0, upload='to_attr',
                    column='area_label', to_column='label')
        # using call_module because it is easier
        table = call_module('v.db.select', map=self.points,
                            separator='pipe', flags='c')
        self.assertMultiLineEqual(table, table_ref)

    def test_area_attrs_to_3d_points(self):
        """Check that values are uploaded to 3D points in areas (dmax=0)"""
        call_module('v.distance', from_=self.points_3d, to=self.areas,
                    dmax=0, upload='to_attr',
                    column='area_label', to_column='label')
        table = call_module('v.db.select', map=self.points_3d,
                            separator='pipe', flags='c')
        self.assertMultiLineEqual(table, table_ref)


if __name__ == '__main__':
    test()
