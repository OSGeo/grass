"""
(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import datetime
import os
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestTRastAlgebra(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        cls.runModule("r.mapcalc", quiet=True, expression="a1 = 1")
        cls.runModule("r.mapcalc", quiet=True, expression="a2 = 2")
        cls.runModule("r.mapcalc", quiet=True, expression="a3 = 3")
        cls.runModule("r.mapcalc", quiet=True, expression="a4 = 4")
        cls.runModule("r.mapcalc", quiet=True, expression="b1 = 5")
        cls.runModule("r.mapcalc", quiet=True, expression="b2 = 6")
        cls.runModule("r.mapcalc", quiet=True, expression="c1 = 7")
        cls.runModule("r.mapcalc", quiet=True, expression="d1 = 8")
        cls.runModule("r.mapcalc", quiet=True, expression="d2 = 9")
        cls.runModule("r.mapcalc", quiet=True, expression="d3 = 10")        
        cls.runModule("r.mapcalc", quiet=True, expression="singletmap = 99")
        cls.runModule("r.mapcalc", quiet=True, expression="singlemap = 100")

        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field")
        tgis.open_new_stds(name="B", type="strds", temporaltype="absolute",
                                         title="B", descr="B", semantic="field")
        tgis.open_new_stds(name="C", type="strds", temporaltype="absolute",
                                         title="B", descr="C", semantic="field")
        tgis.open_new_stds(name="D", type="strds", temporaltype="absolute",
                                         title="D", descr="D", semantic="field")

        tgis.register_maps_in_space_time_dataset(type="raster", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="B", maps="b1,b2",
                                                 start="2001-01-01", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="C", maps="c1",
                                                 start="2001-01-02", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d1,d2,d3",
                                                 start="2001-01-03", increment="1 day", interval=True)                                                 
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,  maps="singletmap", 
                                                start="2001-01-03", end="2001-01-04")
        
    def tearDown(self):
        self.runModule("t.remove", flags="rf", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region 
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", quiet=True)
        cls.runModule("t.unregister", maps="singletmap", quiet=True)
        cls.del_temp_region()

    def test_temporal_conditional_time_dimension_bug(self):
        """Testing the conditional time dimension bug, that uses the time 
            dimension of the conditional statement instead the time dimension 
            of the then/else statement."""
        self.assertModule("t.rast.algebra", expression="R = if({contains}, B == 5, "
                                                       "A - 1,  A + 1)", basename="r", flags="d")
        self.assertModule("t.rast.algebra", expression="R = if({contains}, B == 5, "
                                                       "A - 1,  A + 1)", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 0) # 1 - 1
        self.assertEqual(D.metadata.get_max_max(), 5) # 4 + 1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_simple_arith_hash_1(self):
        """Simple arithmetic test including the hash operator"""

        self.assertModule("t.rast.algebra", expression='R = A + (A {#, equal,l} A)', basename="r", flags="d")
        self.assertModule("t.rast.algebra", expression='R = A + (A {#, equal,l} A)', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))


    def test_simple_arith_td_1(self):
        """Simple arithmetic test"""
       
        self.assertModule("t.rast.algebra", expression='R = A + td(A)', basename="r", flags="d", suffix="time")
        self.assertModule("t.rast.algebra", expression='R = A + td(A)', basename="r", suffix="time")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_td_2(self):
        """Simple arithmetic test"""
       
        self.assertModule("t.rast.algebra", expression='R = A / td(A)', basename="r", flags="d", suffix="gran")
        self.assertModule("t.rast.algebra", expression='R = A / td(A)', basename="r", suffix="gran")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_td_3(self):
        """Simple arithmetic test"""
       
        self.assertModule("t.rast.algebra", expression='R = A {+,equal} td(A)', basename="r", flags="d")
        self.assertModule("t.rast.algebra", expression='R = A {+,equal} td(A)', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_td_4(self):
        """Simple arithmetic test"""
       
        self.assertModule("t.rast.algebra", expression='R = A {/, equal} td(A)', basename="r", flags="d")
        self.assertModule("t.rast.algebra", expression='R = A {/, equal} td(A)', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))


    def test_simple_arith_if_1(self):
        """Simple arithmetic test with if condition"""
       
        self.assertModule("t.rast.algebra", expression='R = if({equal}, start_date(A)'
                                                       ' >= "2001-01-02", A + A)', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 4)
        self.assertEqual(D.metadata.get_max_max(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_if_2(self):
        """Simple arithmetic test with if condition"""
       
        self.assertModule("t.rast.algebra", expression='R = if({equal}, A#A == 1, A - A)', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 0)
        self.assertEqual(D.metadata.get_max_max(), 0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_complex_arith_if_1(self):
        """Complex arithmetic test with if condition"""
       
        self.assertModule("t.rast.algebra", expression='R = if(start_date(A) < "2001-01-03" && A#A == 1,'
                                                        ' A{+, starts,l}C, A{+, finishes,l}C)',
                          basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 9)  # 2 + 7 a2 + c1
        self.assertEqual(D.metadata.get_max_max(), 10) # 3 + 7 a3 + c1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_simple_arith_1(self):
        """Simple arithmetic test"""
       
        self.assertModule("t.rast.algebra", expression="R = A {*, equal} A {+, equal} A", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)  # 1*1 + 1
        self.assertEqual(D.metadata.get_max_max(), 20) # 4*4 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_2(self):
        """Simple arithmetic test that creates an empty strds"""
       
        self.assertModule("t.rast.algebra", expression="R = A {*, during} A {+, during} A", basename="r")
        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 0)

    def test_simple_arith_3(self):
        """Simple arithmetic test"""
       
        self.assertModule("t.rast.algebra", expression="R = A / A + A*A/A", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2) # 1/1 + 1*1/1
        self.assertEqual(D.metadata.get_max_max(), 5) # 4/4 + 4*4/4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        
    def test_temporal_intersection_1(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression="R = A {+,equal,i} B", basename="r")
        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 0)

    def test_temporal_intersection_2(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression="R = A {+,during,i} B", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 6) # 1 + 5
        self.assertEqual(D.metadata.get_max_max(), 10) # 4 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_3(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression="R = A {+,starts,i} B", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 6) # 1 + 5
        self.assertEqual(D.metadata.get_max_max(), 9) # 3 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_intersection_4(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression="R = A {+,finishes,intersect} B", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 7)  # 2 + 5
        self.assertEqual(D.metadata.get_max_max(), 10) # 4 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_5(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression="R = A {+,starts|finishes,i} B", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 6)  # 1 + 5
        self.assertEqual(D.metadata.get_max_max(), 10) # 4 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_6(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression="R = B {+,overlaps,u} C", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 12) # 5 + 7
        self.assertEqual(D.metadata.get_max_max(), 12) # 5 + 7
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_intersection_7(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression="R = B {+,overlapped,u} C", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 13) # 6 + 7
        self.assertEqual(D.metadata.get_max_max(), 13) # 6 + 7
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_8(self):
        """Simple temporal intersection test"""
       
        self.assertModule("t.rast.algebra", expression='R = A {+,during,l} buff_t(C, "1 day") ',
                  basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 8)  # 1 + 7  a1 + c1
        self.assertEqual(D.metadata.get_max_max(), 11) # 4 + 7  a4 + c1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_neighbors_1(self):
        """Simple temporal neighborhood computation test"""
       
        self.assertModule("t.rast.algebra", expression='R = A[-1] + A[1]',
                  basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_neighbors_2(self):
        """Simple temporal neighborhood computation test"""
       
        self.assertModule("t.rast.algebra", expression='R = A[0,0,-1] + A[0,0,1]',
                  basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_tmap_function1(self):
        """Testing the tmap function. """
       
        self.assertModule("t.rast.algebra", expression='R = tmap(singletmap)', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        maplist = D.get_registered_maps_as_objects()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 99) 
        self.assertEqual(D.metadata.get_max_max(), 99) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_tmap_function2(self):
        """Testing the tmap function. """
       
        self.assertModule("t.rast.algebra", expression='R = tmap(singletmap) + 1', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        maplist = D.get_registered_maps_as_objects()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 100) 
        self.assertEqual(D.metadata.get_max_max(), 100) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_map_function1(self):
        """Testing the map function. """
       
        self.assertModule("t.rast.algebra", expression='R = map(singlemap) + A', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        maplist = D.get_registered_maps_as_objects()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 101) 
        self.assertEqual(D.metadata.get_max_max(), 104) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_map_function2(self):
        """Testing the map function. """
       
        self.assertModule("t.rast.algebra", expression='R =  A * map(singlemap)', basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        maplist = D.get_registered_maps_as_objects()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 100) 
        self.assertEqual(D.metadata.get_max_max(), 400) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_temporal_select(self):
        """Testing the temporal select operator. """
       
        self.assertModule("t.rast.algebra", expression="R = A : A", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 4) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_temporal_select(self):
        """Testing the temporal select operator. """
       
        self.assertModule("t.rast.algebra", expression="R = A : D", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 3) 
        self.assertEqual(D.metadata.get_max_max(), 4) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_temporal_select_operators1(self):
        """Testing the temporal select operator. Including temporal relations. """
       
        self.assertModule("t.rast.algebra", expression="R = A : D", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 3) 
        self.assertEqual(D.metadata.get_max_max(), 4) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_temporal_select_operators2(self):
        """Testing the temporal select operator. Including temporal relations. """
       
        self.assertModule("t.rast.algebra", expression="R = A {!:,during} C", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 4) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_temporal_select_operators3(self):
        """Testing the temporal select operator. Including temporal relations and 
            different temporal operators (lr|+&)"""
       
        self.assertModule("t.rast.algebra", expression="R = A {:,during,d} B", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 4) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  u'2 days')

    def test_temporal_select_operators4(self):
        """Testing the temporal select operator. Including temporal relations and 
            different temporal operators (lr|+&)"""
       
        self.assertModule("t.rast.algebra", expression="R = A {:,equal|during,r} C", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        maplist = D.get_registered_maps_as_objects()
        for map_i in maplist:
            start_map, end_map = map_i.get_absolute_time()
            self.assertEqual(start_map, datetime.datetime(2001, 1, 2))
            self.assertEqual(end_map, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 3)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  u'2 days')

    def test_temporal_hash_operator1(self):
        """Testing the temporal hash operator in the raster algebra. """
       
        self.assertModule("t.rast.algebra", expression="R = if(A # D == 1, A)", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 3) 
        self.assertEqual(D.metadata.get_max_max(), 4) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_temporal_hash_operator2(self):
        """Testing the temporal hash operator in the raster algebra. """
       
        self.assertModule("t.rast.algebra", expression="R = A # D", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 1) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_temporal_hash_operator3(self):
        """Testing the temporal hash operator in the raster algebra. """
       
        self.assertModule("t.rast.algebra", expression="R = C {#,contains} A", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 2) 
        self.assertEqual(D.metadata.get_max_max(), 2) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'2 days')

    def test_temporal_hash_operator4(self):
        """Testing the temporal hash operator in the raster algebra. """
       
        self.assertModule("t.rast.algebra", expression="R = if({contains},A # D == 1, C {#,contains} A)", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 2) 
        self.assertEqual(D.metadata.get_max_max(), 2) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'2 days')
 
    def test_raster_arithmetic_relation_1(self):
        """Arithmetic test with temporal intersection"""
       
        self.assertModule("t.rast.algebra", expression="R = B {+,contains,l} A ", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 8) 
        self.assertEqual(D.metadata.get_max_max(), 13) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'2 days')

    def test_raster_arithmetic_relation_2(self):
        """Arithmetic test with temporal intersection"""
       
        self.assertModule("t.rast.algebra", expression="R = B {*,contains,l} A ", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 10) 
        self.assertEqual(D.metadata.get_max_max(), 72) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'2 days')

    def test_raster_arithmetic_relation_3(self):
        """Arithmetic test with temporal intersection"""
       
        self.assertModule("t.rast.algebra", expression="R = B {+,contains,l} A ", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 8) 
        self.assertEqual(D.metadata.get_max_max(), 13) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'2 days')
    
    def test_raster_arithmetic_relation_4(self):
        """Arithmetic test with temporal intersection"""

        self.assertModule("t.rast.algebra", expression="R = B {+,contains,r} A ", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(),4)
        self.assertEqual(D.metadata.get_min_min(), 8) 
        self.assertEqual(D.metadata.get_max_max(), 13) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')
    
    def test_raster_arithmetic_relation_5(self):
        """Complex arithmetic test with temporal intersection"""
       
        self.assertModule("t.rast.algebra", expression="R = tmap(singletmap) "
                                                        "{+,equal| precedes| follows,l} "
                                                        "A + map(singlemap)", basename="r")

        D = tgis.open_old_stds("R", type="strds")
        
        self.assertEqual(D.metadata.get_number_of_maps(),1)
        self.assertEqual(D.metadata.get_min_min(), 208) 
        self.assertEqual(D.metadata.get_max_max(), 208)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')


if __name__ == '__main__':
    test()
