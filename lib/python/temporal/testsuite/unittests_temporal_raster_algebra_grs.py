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


class TestTemporalRasterAlgebra(TestCase):

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
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="nullmap = null()")

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
        tgis.register_maps_in_space_time_dataset(type="raster", name=None, maps="singletmap",
                                                start="2001-03-01", end="2001-04-01")
        tgis.register_maps_in_space_time_dataset(type="raster", name=None, maps="nullmap",
                                                start="2001-01-01", end="2001-07-01")
        
    def tearDown(self):
        return
        self.runModule("t.remove", flags="rf", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        return
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", quiet=True)
        cls.runModule("t.unregister", maps="singletmap", quiet=True)
        cls.del_temp_region()

    def test_1(self):
        """Simple arithmetik test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = "R = if(C == 9,  A - 1)"
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)
        
        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 0) # 1 - 1
        self.assertEqual(D.metadata.get_max_max(), 5) # 6 - 1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 month')

        tra = tgis.TemporalRasterAlgebraParser(run=True, debug=True, dry_run=True)
        tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        pc = tra.parse(expression=expr, basename="r", overwrite=True)

        self.assertEqual(len(pc["register"]), 6)
        self.assertEqual(len(pc["processes"]), 6)
        self.assertEqual(pc["processes"][0]["name"], "r.mapcalc")
        self.assertEqual(pc["processes"][5]["name"], "r.mapcalc")
        self.assertEqual(pc["STDS"]["name"], "R")
        self.assertEqual(pc["STDS"]["stdstype"], "strds")

    def test_2(self):
        """Simple arithmetik test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = "R = A + B + C"
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 17) # 1 + 7 + 9
        self.assertEqual(D.metadata.get_max_max(), 23) # 6 + 8 + 9
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 month')

        tra = tgis.TemporalRasterAlgebraParser(run=True, debug=True, dry_run=True)
        tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        pc = tra.parse(expression=expr, basename="r", overwrite=True)

        self.assertEqual(len(pc["register"]), 6)
        self.assertEqual(len(pc["processes"]), 6)
        self.assertEqual(pc["processes"][0]["name"], "r.mapcalc")
        self.assertEqual(pc["processes"][5]["name"], "r.mapcalc")
        self.assertEqual(pc["STDS"]["name"], "R")
        self.assertEqual(pc["STDS"]["stdstype"], "strds")

    def test_3(self):
        """Simple arithmetik test with null map"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = "R = A + B + C + tmap(nullmap)"
        ret = tra.setup_common_granularity(expression=expr,
                                           lexer=tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 0)
        self.assertEqual(D.metadata.get_min_min(), None)
        self.assertEqual(D.metadata.get_max_max(), None)
        start, end = D.get_absolute_time()
        self.assertEqual(start, None)
        self.assertEqual(end, None)
        self.assertEqual(D.check_temporal_topology(), False)
        self.assertEqual(D.get_granularity(), None)

        tra = tgis.TemporalRasterAlgebraParser(run=True, debug=True, dry_run=True)
        tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        pc = tra.parse(expression=expr, basename="r", overwrite=True)

        print(pc)

        self.assertEqual(len(pc["register"]), 0)
        self.assertEqual(len(pc["processes"]), 0)
        self.assertEqual(pc["STDS"]["name"], "R")
        self.assertEqual(pc["STDS"]["stdstype"], "strds")

    def test_4(self):
        """Simple arithmetik test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = "R = if(D == 11,  A - 1, A + 1)"
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 15)
        self.assertEqual(D.metadata.get_min_min(), 2) # 1 - 1
        self.assertEqual(D.metadata.get_max_max(), 6) # 5 + 1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 5, 6))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_simple_arith_hash_1(self):
        """Simple arithmetic test including the hash operator"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = A + (A # A)', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 7)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_simple_arith_hash_2(self):
        """Simple arithmetic test including the hash operator"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = (A + A) # A', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 1)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_simple_arith_td_1(self):
        """Simple arithmetic test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = 'R = A + td(A:D)'
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 15)
        self.assertEqual(D.metadata.get_min_min(), 2) # 1 - 1
        self.assertEqual(D.metadata.get_max_max(), 6) # 5 + 1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 5, 6))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_simple_arith_if_1(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = 'R = if(start_date(A) >= "2001-02-01", A + A)'
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 5)
        self.assertEqual(D.metadata.get_min_min(), 4)
        self.assertEqual(D.metadata.get_max_max(), 12)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_simple_arith_if_2(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = 'R = if(A#A == 1, A - A)'
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 0)
        self.assertEqual(D.metadata.get_max_max(), 0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_complex_arith_if_1(self):
        """Complex arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = 'R = if(start_date(A) < "2001-03-01" && A#A == 1, A+C, A-C)'
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), -6)  # 3 - 9
        self.assertEqual(D.metadata.get_max_max(), 11) # 2 + 2
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_temporal_neighbors(self):
        """Simple temporal neighborhood computation test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = 'R = (A[0,0,-1] : D) + (A[0,0,1] : D)'
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 14)
        self.assertEqual(D.metadata.get_min_min(), 2)  # 1 + 1
        self.assertEqual(D.metadata.get_max_max(), 10) # 5 + 5
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 5, 6))

    def test_map(self):
        """Test STDS + single map without timestamp"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = "R = A + map(singletmap)"
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(), 6)
        self.assertEqual(D.metadata.get_min_min(), 100)  # 1 + 99
        self.assertEqual(D.metadata.get_max_max(), 105) # 6 + 99
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 7, 1))

    def test_tmap_map(self):
        """Test STDS + single map with and without timestamp"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        expr = "R = tmap(singletmap) + A + map(singletmap)"
        ret = tra.setup_common_granularity(expression=expr, lexer = tgis.TemporalRasterAlgebraLexer())
        self.assertEqual(ret, True)

        tra.parse(expression=expr, basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")

        self.assertEqual(D.metadata.get_number_of_maps(),1)
        self.assertEqual(D.metadata.get_min_min(), 201)
        self.assertEqual(D.metadata.get_max_max(), 201)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 3, 1))
        self.assertEqual(end, datetime.datetime(2001, 4, 1))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 month')


if __name__ == '__main__':
    test()
