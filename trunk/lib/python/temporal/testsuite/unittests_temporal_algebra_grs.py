"""
(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""

import grass.script
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import datetime
import os

class TestTemporalAlgebraGranularity(TestCase):

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
                                                 start="2001-01-01", increment="31 days", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d2",
                                                 start="2001-03-01", increment="31 days", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="D", maps="d3",
                                                 start="2001-05-01", increment="31 days", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,  maps="singletmap", 
                                                start="2001-01-03", end="2001-01-04")

    def tearDown(self):
        pass
        #self.runModule("t.remove", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        #cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", quiet=True)
        cls.del_temp_region()

    def test_common_granularity_1(self):
        """Testing the common granularity function. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        expr = 'R = A : B'
        ret = ta.setup_common_granularity(expression=expr)

        self.assertEqual(ret, True)
        self.assertEqual(ta.granularity, "1 month")

        ta.count = 0
        ta.stdstype = "strds"
        ta.maptype = "raster"
        ta.mapclass = tgis.RasterDataset

        maplist = ta.check_stds("A")
        self.assertEqual(len(maplist), 6)
        maplist = ta.check_stds("B")
        self.assertEqual(len(maplist), 6)
        
        ta.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 6) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')
       
    def test_common_granularity_2(self):
        """Testing the common granularity function year to month samping. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        expr = 'R = A : C'
        ret = ta.setup_common_granularity(expression=expr)
        
        ta.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 6) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')
       

    def test_common_granularity_3(self):
        """Testing the common granularity function with gaps. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        expr = 'R = A : D'
        ret = ta.setup_common_granularity(expression=expr)
        
        ta.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 5) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 6, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')
       

    def test_common_granularity_4(self):
        """Testing the common granularity function year to month with gaps. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        expr = 'R = C : D'
        ret = ta.setup_common_granularity(expression=expr)
        
        ta.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 9) 
        self.assertEqual(D.metadata.get_max_max(), 9) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 6, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')
       
    def test_common_granularity_4(self):
        """Testing the common granularity function year to month with gaps. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        expr = 'R = C : D'
        ret = ta.setup_common_granularity(expression=expr)
        
        ta.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 9) 
        self.assertEqual(D.metadata.get_max_max(), 9) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 6, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')
       
    def test_common_granularity_5(self):
        """Testing the common granularity function year to month with gaps. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        expr = 'R = A : C : D'
        ret = ta.setup_common_granularity(expression=expr)
        
        ta.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 1) 
        self.assertEqual(D.metadata.get_max_max(), 5) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 6, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')
         
    def test_common_granularity_6(self):
        """Testing the common granularity function year to month with gaps. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        expr = 'R = if(start_month(A) > 2, A : C : D)'
        ret = ta.setup_common_granularity(expression=expr)
        
        ta.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 3) 
        self.assertEqual(D.metadata.get_max_max(), 5) 
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 3, 1))
        self.assertEqual(end, datetime.datetime(2001, 6, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  u'1 month')
       


if __name__ == '__main__':
    test()
