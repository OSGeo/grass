"""
(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""

import datetime
import os
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestTRastAlgebraGranularity(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a5 = 5")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a6 = 6")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b1 = 7")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b2 = 8")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="c1 = 9")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="d1 = 10")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="d2 = 11")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="d3 = 12")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="singletmap = 99")

        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)
        tgis.open_new_stds(name="B", type="strds", temporaltype="absolute",
                                         title="B", descr="B", semantic="field", overwrite=True)
        tgis.open_new_stds(name="C", type="strds", temporaltype="absolute",
                                         title="C", descr="C", semantic="field", overwrite=True)
        tgis.open_new_stds(name="D", type="strds", temporaltype="absolute",
                                         title="D", descr="D", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="A", maps="a1,a2,a3,a4,a5,a6",
                                                 start="2001-01-01", increment="1 month", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="B", maps="b1,b2",
                                                 start="2001-01-01", increment="3 months", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="C", maps="c1",
                                                 start="2001-01-01", increment="1 year", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d1",
                                                 start="2001-01-01", increment="5 days", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d2",
                                                 start="2001-03-01", increment="5 days", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d3",
                                                 start="2001-05-01", increment="5 days", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,  maps="singletmap", 
                                                start="2001-03-01", end="2001-04-01")
        
    def tearDown(self):
        self.runModule("t.remove", flags="rf", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region 
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", quiet=True)
        cls.runModule("t.unregister", maps="singletmap", quiet=True)
        cls.del_temp_region()

    def test_1(self):
        """Simple arithmetik test"""
        expr = "R = if(C == 9,  A - 1)"
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 0) # 1 - 1
        self.assertEqual(D.metadata.get_max_max(), 5) # 6 - 1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')

    def test_2(self):
        """Simple arithmetik test"""
        expr = "R = if(D == 11,  A - 1, A + 1)"
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 15)
        self.assertEqual(D.metadata.get_min_min(), 2) # 1 - 1
        self.assertEqual(D.metadata.get_max_max(), 6) # 5 + 1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 5, 6))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_simple_arith_hash_1(self):
        """Simple arithmetic test including the hash operator"""
        expr ='R = A + (A # A)'
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")
        
        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 7)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))


    def test_simple_arith_td_1(self):
        """Simple arithmetic test"""
        expr = 'R = A + td(A:D)'
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")
        
        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 15)
        self.assertEqual(D.metadata.get_min_min(), 2) # 1 - 1
        self.assertEqual(D.metadata.get_max_max(), 6) # 5 + 1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 5, 6))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_simple_arith_if_1(self):
        """Simple arithmetic test with if condition"""
        expr = 'R = if(start_date(A) >= "2001-02-01", A + A)'
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 5)
        self.assertEqual(D.metadata.get_min_min(), 4)
        self.assertEqual(D.metadata.get_max_max(), 12)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_simple_arith_if_2(self):
        """Simple arithmetic test with if condition"""
        expr = 'R = if(A#A == 1, A - A)'
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 0)
        self.assertEqual(D.metadata.get_max_max(), 0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_complex_arith_if_1(self):
        """Complex arithmetic test with if condition"""
        expr = 'R = if(start_date(A) < "2001-03-01" && A#A == 1, A+C, A-C)'
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), -6)  # 3 - 9
        self.assertEqual(D.metadata.get_max_max(), 11) # 2 + 2
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_temporal_neighbors(self):
        """Simple temporal neighborhood computation test"""
        expr ='R = (A[0,0,-1] : D) + (A[0,0,1] : D)'
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 14)
        self.assertEqual(D.metadata.get_min_min(), 2)  # 1 + 1
        self.assertEqual(D.metadata.get_max_max(), 10) # 5 + 5
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 5, 6))
    
    def test_map(self):
        """Test STDS + single map without timestamp"""
        expr = "R = A + map(singletmap)"
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 100)  # 1 + 99
        self.assertEqual(D.metadata.get_max_max(), 105) # 6 + 99
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))
        
    def test_tmap_map(self):
        """Test STDS + single map with and without timestamp"""
        expr = "R = tmap(singletmap) + A + map(singletmap)"
        self.assertModule("t.rast.algebra",  expression=expr, flags="gd", basename="r")
        self.assertModule("t.rast.algebra",  expression=expr, flags="g", basename="r")

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(),1)
        self.assertEqual(D.metadata.get_min_min(), 201) 
        self.assertEqual(D.metadata.get_max_max(), 201)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 3, 1))
        self.assertEqual(end, datetime.datetime(2001, 4, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')


if __name__ == '__main__':
    test()
