"""
(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Thomas leppelt and Soeren Gebbert
"""

import grass.script
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import datetime
import os

class TestTemporalConditionals(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b1 = 5")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b2 = 6")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="c1 = 7")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="d1 = 8")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="d2 = 9")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="d3 = 10")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="e1 = 11")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="e2 = 12")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="e3 = 13")

        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="singletmap = 99")

        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)
        tgis.open_new_stds(name="B", type="strds", temporaltype="absolute",
                                         title="B", descr="B", semantic="field", overwrite=True)
        tgis.open_new_stds(name="C", type="strds", temporaltype="absolute",
                                         title="C", descr="C", semantic="field", overwrite=True)
        tgis.open_new_stds(name="D", type="strds", temporaltype="absolute",
                                         title="D", descr="D", semantic="field", overwrite=True)
        tgis.open_new_stds(name="E", type="strds", temporaltype="absolute",
                                         title="E", descr="E", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="B", maps="b1,b2",
                                                 start="2001-01-01", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="C", maps="c1",
                                                 start="2001-01-02", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d1,d2,d3",
                                                 start="2001-01-03", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="E", maps="e1,e2,e3",
                                                 start="2000-12-31", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name=None, maps="singletmap",
                                                start="2001-01-03", end="2001-01-04")

    def tearDown(self):
        self.runModule("t.remove", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D,E", quiet=True)
        cls.del_temp_region()

    def test_temporal_condition_1(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(A) >= "2001-01-03", A)', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 3)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_2(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(td(A) == 1, A)', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_3(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(td(A) == 1 || start_date(A) >= "2001-01-03", A)', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_4(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(A) >= "2001-01-03", A)', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 3)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_5(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_day(A) <= 2, A)', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 2)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_6(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(td(A) == 1 {||,during} start_date(C) < "2001-01-02", A)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 3)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_7(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if({over},start_date(C) == "2001-01-02" {&&,contains} td(A) == 1, B)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 5)
        self.assertEqual(D.metadata.get_max_max(), 6)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_condition_8(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(B) <= "2001-01-01" {||,over,|} td(E) == 2, B)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 5)
        self.assertEqual(D.metadata.get_max_max(), 6)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_condition_9(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(B) <= "2001-01-01" {&&,over,&} td(E) == 2, B)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 5)
        self.assertEqual(D.metadata.get_max_max(), 5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_condition_10(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(B) <= "2001-01-01" {||,over,|,r} td(E) == 2, E)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 11)
        self.assertEqual(D.metadata.get_max_max(), 13)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2000, 12, 31))
        self.assertEqual(end, datetime.datetime(2001, 1, 6))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_condition_11(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(B) <= "2001-01-01" {&&,over,r} td(E) == 2, E)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 11)
        self.assertEqual(D.metadata.get_max_max(), 11)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2000, 12, 31))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_condition_12(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(B) <= "2001-01-01" {&&,over,|,r} td(E) == 2, E)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 11)
        self.assertEqual(D.metadata.get_max_max(), 11)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2000, 12, 31))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_conditional_13(self):
        """Testing the hash operator function in conditional statement. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if({equal|during},td(B) == 2 {&&,contains} td(A) == 1, A)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        maplist = D.get_registered_maps_as_objects()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_else_1(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(start_date(A) <= "2001-01-03", A, D)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 9)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_else_2(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if(td(D) == 1 && start_date(A) >= "2001-01-04", A, D)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)
        self.assertEqual(D.metadata.get_max_max(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_else_3(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if({during},td(B) == 2 {&&,contains} start_date(D) >= "2001-01-04", A, D)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 8)
        self.assertEqual(D.metadata.get_max_max(), 9)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_condition_else_4(self):
        """Testing the temporal select operator with equal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression='R = if({equal|over},start_date(B) <= "2001-01-01" {&&,over,|,r} td(E) == 2, E, B)',
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()
        for map in D.get_registered_maps_as_objects():
            print(map.get_map_id())
        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 5)
        self.assertEqual(D.metadata.get_max_max(), 11)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2000, 12, 31))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), False)
        self.assertEqual(D.get_granularity(), u'2 days')

if __name__ == '__main__':
    test()
