"""
(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Thomas Leppelt
"""

import datetime
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestTemporalRasterAlgebraConditionals(TestCase):

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
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="d4 = 11")

        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)
        tgis.open_new_stds(name="B", type="strds", temporaltype="absolute",
                                         title="B", descr="B", semantic="field", overwrite=True)
        tgis.open_new_stds(name="C", type="strds", temporaltype="absolute",
                                         title="C", descr="C", semantic="field", overwrite=True)
        tgis.open_new_stds(name="D", type="strds", temporaltype="absolute",
                                         title="D", descr="D", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="B", maps="b1,b2",
                                                 start="2001-01-01", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="C", maps="c1",
                                                 start="2001-01-02", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d1,d2,d3,d4",
                                                 start="2001-01-03", increment="1 day", interval=True)

    def tearDown(self):
        self.runModule("t.remove", flags="rf", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", quiet=True)
        cls.del_temp_region()

    def test_temporal_conditional_time_dimension_bug(self):
        """Testing the conditional time dimension bug, that uses the time
            dimension of the conditional statement instead the time dimension
            of the then/else statement."""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains}, B == 5,  A - 1,  A + 1)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0) # 1 - 1
        self.assertEqual(R.metadata.get_max_max(), 5) # 4 + 1
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_temporal_conditional_1(self):
        """Testing the conditional time dimension bug, that uses the time
            dimension of the conditional statement instead the time dimension
            of the then/else statement."""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(td(A) == 1,  D * 2, D)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 2)
        self.assertEqual(R.metadata.get_min_min(), 16)
        self.assertEqual(R.metadata.get_max_max(), 18)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1,  3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_temporal_conditional_relation_1(self):
        """Testing the conditional time dimension bug, that uses the time
            dimension of the conditional statement instead the time dimension
            of the then/else statement."""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({during},exist(A),  B - 1,  B + 1)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 4)
        self.assertEqual(R.metadata.get_max_max(), 5)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  False)
        self.assertEqual(R.get_granularity(),  u'2 days')

    def test_spatial_conditional_1(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 1 && A < 4 && isntnull(A), A)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 3)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_2(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 1 && A < 4 && isntnull(A), A, null())', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 2)
        self.assertEqual(R.metadata.get_min_min(), 2)
        self.assertEqual(R.metadata.get_max_max(), 3)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_3(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 1, A, D)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 2)
        self.assertEqual(R.metadata.get_min_min(), 3)
        self.assertEqual(R.metadata.get_max_max(), 4)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_4(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 0, A)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 1)
        self.assertEqual(R.metadata.get_max_max(), 4)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_5(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(B > 5 {&&,contains,l} A < 5, B)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 2)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 6)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'2 days')

    def test_spatial_conditional_relation_1(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains},B > 5, D)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 2)
        self.assertEqual(R.metadata.get_min_min(), 8)
        self.assertEqual(R.metadata.get_max_max(), 9)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_relation_2(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains}, B <= 5, A, A * 2)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 1)
        self.assertEqual(R.metadata.get_max_max(), 8)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_numeric_relation_1(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains}, B > 5, A, 10)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 3)
        self.assertEqual(R.metadata.get_max_max(), 10)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_numeric_relation_2(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains},B > 5, A + 2 / 4.0)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 4.5)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_numeric_1(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 2, 0, A)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 2)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_numeric_2(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 2, A, 8)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 3)
        self.assertEqual(R.metadata.get_max_max(), 8)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_numeric_3(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 2, 1, 0)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 1)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatial_conditional_numeric_4(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 2, null())', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 2)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 0)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_1(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A < 2 && start_date(A) < "2001-01-03", A)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 1)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_2(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A < 3 || start_date(A) < "2001-01-04", A + 1, A - 1)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(),2)
        self.assertEqual(R.metadata.get_max_max(),4)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_relation_1(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains},B > 5 && start_day(B) < 3, D)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 2)
        self.assertEqual(R.metadata.get_min_min(),0)
        self.assertEqual(R.metadata.get_max_max(),0)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_relation_2(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains}, start_date(B) < "2001-01-03" || B <= 5, A, A * 2)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 1)
        self.assertEqual(R.metadata.get_max_max(), 8)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_numeric_relation_1(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains}, start_date(B) >= "2001-01-03" && B > 5, A, 10)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 3)
        self.assertEqual(R.metadata.get_max_max(), 10)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_numeric_relation_2(self):
        """Testing the spatial conditionals combined by AND/OR operators.
            Evaluation  """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({contains},td(B) == 2 && start_date(B) == "2001-01-03" && B > 5 , A + 2 / 4.0)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 4.5)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_numeric_1(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(start_date(A) < "2001-01-04" && A > 2, 0, A)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 4)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_numeric_2(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 2 || start_date(A) > "2001-01-01" && start_date(A) < "2001-01-04", A, 8)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 2)
        self.assertEqual(R.metadata.get_max_max(), 8)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_numeric_3(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(start_date(A) < "2001-01-04" && A > 2, 1, 0)', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 1)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_spatiotemporal_conditional_numeric_4(self):
        """Testing the spatial conditionals with numeric conclusions"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if(A > 2 || start_date(A) > "2001-01-01", null())', basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 1)
        self.assertEqual(R.metadata.get_min_min(), 0)
        self.assertEqual(R.metadata.get_max_max(), 0)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')


if __name__ == '__main__':
    test()
