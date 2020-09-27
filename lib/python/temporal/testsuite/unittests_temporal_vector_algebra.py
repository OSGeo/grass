"""
(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""

import datetime
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestTemporalVectorAlgebra(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=1, output='a1')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=1, output='a2')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=1, output='a3')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=1, output='a4')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=2, output='b1')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=2, output='b2')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=3, output='c1')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=4, output='d1')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=4, output='d2')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=4, output='d3')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=5, output='singletmap')
        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=6, output='singlemap')

        tgis.open_new_stds(name="A", type="stvds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)
        tgis.open_new_stds(name="B", type="stvds", temporaltype="absolute",
                                         title="B", descr="B", semantic="field", overwrite=True)
        tgis.open_new_stds(name="C", type="stvds", temporaltype="absolute",
                                         title="B", descr="C", semantic="field", overwrite=True)
        tgis.open_new_stds(name="D", type="stvds", temporaltype="absolute",
                                         title="D", descr="D", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="vector", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="vector", name="B", maps="b1,b2",
                                                 start="2001-01-01", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="vector", name="C", maps="c1",
                                                 start="2001-01-02", increment="2 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="vector", name="D", maps="d1,d2,d3",
                                                 start="2001-01-03", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="vector", name=None, maps="singletmap",
                                                start="2001-01-03", end="2001-01-04")
    
    def tearDown(self):
        self.runModule("t.remove", type="stvds", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", type='stvds', quiet=True)
        cls.del_temp_region()

    def test_temporal_select(self):
        """Testing the temporal select operator. """
        tva = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        tva.parse(expression="R = A : A", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_number_of_points(), 80)
        self.assertEqual(D.metadata.get_number_of_areas(), 0)
        self.assertEqual(D.metadata.get_number_of_centroids(), 0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_extent1(self):
        """Testing the temporal extent operators. """
        ta = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        ta.parse(expression="R = A {:,during,r} C", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        D.print_info()
        maplist = D.get_registered_maps_as_objects()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), False)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_select_operators(self):
        """Testing the temporal select operator. Including temporal relations. """
        tva = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        tva.parse(expression="R = A {:,during} C", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_number_of_points(), 40)
        self.assertEqual(D.metadata.get_number_of_areas(), 0)
        self.assertEqual(D.metadata.get_number_of_centroids(), 0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_buff_operators_1(self):
        """Testing the bufferoperator."""
        tva = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        tva.parse(expression="R = buff_p(A,0.5)", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_number_of_points(), 0)
        self.assertEqual(D.metadata.get_number_of_areas(), 80)
        self.assertEqual(D.metadata.get_number_of_centroids(), 80)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_buff_operators_2(self):
        """Testing the bufferoperator."""
        tva = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        tva.parse(expression="R = buff_a(buff_p(A,1),10)", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_number_of_points(), 0)
        self.assertEqual(D.metadata.get_number_of_areas(), 20)
        self.assertEqual(D.metadata.get_number_of_centroids(), 20)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_overlay_operators_1(self):
        """Testing the spatial overlay operator."""
        tva = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        tva.parse(expression="R = buff_p(A,2) & buff_p(D,2)", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_number_of_points(), 0)
        self.assertEqual(D.metadata.get_number_of_areas(), 6)
        self.assertEqual(D.metadata.get_number_of_centroids(), 6)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_overlay_operators_2(self):
        """Testing the spatial overlay operator."""
        tva = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        tva.parse(expression="R = buff_p(A,1.5) {&,during,r} buff_p(B,1.5)", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_number_of_points(), 0)
        self.assertEqual(D.metadata.get_number_of_areas(), 8)
        self.assertEqual(D.metadata.get_number_of_centroids(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), False)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_overlay_operators_3(self):
        """Testing the spatial overlay operator."""
        tva = tgis.TemporalVectorAlgebraParser(run = True, debug = True)
        tva.parse(expression="R = buff_p(A,2.5) {&,during,l} buff_p(C,2.5)", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="stvds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_number_of_points(), 0)
        self.assertEqual(D.metadata.get_number_of_areas(), 8)
        self.assertEqual(D.metadata.get_number_of_centroids(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')


if __name__ == '__main__':
    test()
