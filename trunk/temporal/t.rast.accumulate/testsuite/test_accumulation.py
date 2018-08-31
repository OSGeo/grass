"""Test t.rast.accumulate

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import grass.temporal as tgis
import datetime
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

class TestAccumulate(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)

        cls.runModule("r.mapcalc", expression="a_1 = 5",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_2 = 10",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_3 = 20",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_4 = 35",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_5 = 20",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_6 = 10",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_7 = 5",  overwrite=True)


        cls.runModule("r.mapcalc", expression="lower = 10",  overwrite=True)
        cls.runModule("r.mapcalc", expression="upper = 30",  overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="absolute",
                                   output="A",  title="A test",
                                   description="A test", overwrite=True)
        cls.runModule("t.register",  flags="i",  type="raster",  input="A",
                                     maps="a_1,a_2,a_3,a_4,a_5,a_6,a_7",
                                     start="2001-01-01",
                                     increment="1 day",  overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="absolute",
                                   output="Lower",  title="Lower",
                                   description="Lower", overwrite=True)
        cls.runModule("t.register",  type="raster",  input="Lower",
                                     maps="lower",
                                     start="2001-01-01",
                                     end="2001-01-10",
                                     overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="absolute",
                                   output="Upper",  title="Upper",
                                   description="Upper", overwrite=True)
        cls.runModule("t.register",  type="raster",  input="Upper",
                                     maps="upper",
                                     start="2001-01-01",
                                     end="2001-01-10",
                                     overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove",  flags="rf",  type="strds",
                                   inputs="A")
        cls.runModule("t.remove",  flags="rf",  type="strds",
                                   inputs="Lower")
        cls.runModule("t.remove",  flags="rf",  type="strds",
                                   inputs="Upper")
        cls.del_temp_region()

    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove", flags="rf", type="strds", inputs="B")

    def test_1(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[0,40], method="gdd",
                          start="2001-01-01", cycle="7 days",
                          basename="b",
                          overwrite=True,  verbose=True)

        D = tgis.open_old_stds("B", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 7)
        self.assertEqual(D.metadata.get_min_min(), 5)
        self.assertEqual(D.metadata.get_max_max(), 105)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 8))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_2(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[10,40], method="gdd",
                          start="2001-01-01", cycle="7 days",
                          basename="b",
                          overwrite=True,  verbose=True)

        D = tgis.open_old_stds("B", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 7)
        self.assertEqual(D.metadata.get_min_min(), 0.0)
        self.assertEqual(D.metadata.get_max_max(), 45.0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 8))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_3(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[10,30], method="bedd",
                          start="2001-01-01", cycle="7 days",
                          basename="b",
                          overwrite=True,  verbose=True)

        D = tgis.open_old_stds("B", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 7)
        self.assertEqual(D.metadata.get_min_min(), 0.0)
        self.assertEqual(D.metadata.get_max_max(), 40.0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 8))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_3_a(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          lower="Lower",
                          upper="Upper",
                          limits=[0,40],
                          method="bedd",
                          start="2001-01-01", cycle="7 days",
                          basename="b",
                          overwrite=True,  verbose=True)

        D = tgis.open_old_stds("B", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 7)
        self.assertEqual(D.metadata.get_min_min(), 0.0)
        self.assertEqual(D.metadata.get_max_max(), 40.0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 8))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_4(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[10,30], method="bedd",
                          start="2001-01-01", cycle="7 days",
                          basename="b", granularity="2 days",
                          overwrite=True,  verbose=True)

        D = tgis.open_old_stds("B", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 0.0)
        self.assertEqual(D.metadata.get_max_max(), 22.5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 9))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'2 days')

    def test_4_a(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          lower="Lower",
                          upper="Upper",
                          limits=[0,40],
                          method="bedd",
                          start="2001-01-01", cycle="7 days",
                          basename="b", granularity="2 days",
                          overwrite=True,  verbose=True)

        D = tgis.open_old_stds("B", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 0.0)
        self.assertEqual(D.metadata.get_max_max(), 22.5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 9))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'2 days')

    def test_5(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[0,40], method="gdd",
                          start="2001-01-01", cycle="7 days",
                          basename="b", stop="2001-01-05",
                          overwrite=True,  verbose=True)

        D = tgis.open_old_stds("B", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 5)
        self.assertEqual(D.metadata.get_max_max(), 70)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 day')

    def test_count_suffix(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[0,40], method="gdd",
                          start="2001-01-01", cycle="7 days",
                          basename="b", stop="2001-01-05", suffix="num",
                          overwrite=True,  verbose=True)
        self.assertRasterExists('b_00001')
        self.assertRasterDoesNotExist('b_2001_01_07')

    def test_count3_suffix(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[0,40], method="gdd",
                          start="2001-01-01", cycle="7 days",
                          basename="b", stop="2001-01-05", suffix="num%03",
                          overwrite=True,  verbose=True)
        self.assertRasterExists('b_001')
        self.assertRasterDoesNotExist('b_2001_01_07')

    def test_time_suffix(self):
        self.assertModule("t.rast.accumulate",  input="A", output="B",
                          limits=[0,40], method="gdd",
                          start="2001-01-01", cycle="7 days",
                          basename="b", stop="2001-01-05", suffix="time",
                          overwrite=True,  verbose=True)
        self.assertRasterExists('b_2001_01_01T00_00_00')

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
