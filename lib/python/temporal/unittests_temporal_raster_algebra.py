"""!Unit test to register raster maps with absolute and relative
   time using tgis.register_maps_in_space_time_dataset()

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import grass.script
import grass.temporal as tgis
import unittest
import datetime
import os

class TestRegisterFunctions(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """!Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        grass.script.use_temp_region()
        ret = grass.script.run_command("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        ret += grass.script.run_command("r.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        ret += grass.script.run_command("r.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        ret += grass.script.run_command("r.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        ret += grass.script.run_command("r.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")
        ret += grass.script.run_command("r.mapcalc", overwrite=True, quiet=True, expression="b1 = 5")
        ret += grass.script.run_command("r.mapcalc", overwrite=True, quiet=True, expression="b2 = 6")
        ret += grass.script.run_command("r.mapcalc", overwrite=True, quiet=True, expression="c1 = 7")


        tgis.open_new_space_time_dataset(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)
        tgis.open_new_space_time_dataset(name="B", type="strds", temporaltype="absolute",
                                         title="B", descr="B", semantic="field", overwrite=True)
        tgis.open_new_space_time_dataset(name="C", type="strds", temporaltype="absolute",
                                         title="B", descr="C", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="rast", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="rast", name="B", maps="b1,b2",
                                                 start="2001-01-01", increment="2 day", interval=True)

        tgis.register_maps_in_space_time_dataset(type="rast", name="C", maps="c1",
                                                 start="2001-01-02", increment="2 day", interval=True)

    def test_simple_arith_td_1(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A + td(A)', basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))


    def test_simple_arith_td_2(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A / td(A)', basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_td_3(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A {equal,+} td(A)', basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))


    def test_simple_arith_td_4(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A {equal,/} td(A)', basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))


    def test_simple_arith_if_1(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = if({equal}, start_date() >= "2001-01-02", A + A)', basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 3)
        self.assertEqual(D.metadata.get_min_min(), 4)
        self.assertEqual(D.metadata.get_max_max(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_if_2(self):
        """Simple arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = if({equal}, A#A == 1, A - A)', basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 0)
        self.assertEqual(D.metadata.get_max_max(), 0)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_complex_arith_if_1(self):
        """Complex arithmetic test with if condition"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = if(start_date() < "2001-01-03" && A#A == 1, A{starts,=+}C, A{finishes,=+}C)', \
                  basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 9)  # 2 + 7 a2 + c1
        self.assertEqual(D.metadata.get_max_max(), 10) # 3 + 7 a3 + c1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_simple_arith_1(self):
        """Simple arithmetic test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A {equal,*} A {equal,+} A", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)  # 1*1 + 1
        self.assertEqual(D.metadata.get_max_max(), 20) # 4*4 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arith_2(self):
        """Simple arithmetic test that creates an empty strds"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A {during,*} A {during,+} A", basename="d", overwrite=True)
        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 0)

    def test_simple_arith_3(self):
        """Simple arithmetic test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A / A + A*A/A", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2) # 1/1 + 1*1/1
        self.assertEqual(D.metadata.get_max_max(), 5) # 4/4 + 4*4/4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_1(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A {equal,&+} B", basename="d", overwrite=True)
        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 0)

    def test_temporal_intersection_2(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A {during,&+} B", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 6) # 1 + 5
        self.assertEqual(D.metadata.get_max_max(), 10) # 4 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_3(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A {starts,&+} B", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 6) # 1 + 5
        self.assertEqual(D.metadata.get_max_max(), 9) # 3 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_intersection_4(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A {finishes,&+} B", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 7)  # 2 + 5
        self.assertEqual(D.metadata.get_max_max(), 10) # 4 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_5(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = A {starts|finishes,&+} B", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 6)  # 1 + 5
        self.assertEqual(D.metadata.get_max_max(), 10) # 4 + 6
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_6(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = B {overlaps,|+} C", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 12) # 5 + 7
        self.assertEqual(D.metadata.get_max_max(), 12) # 5 + 7
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_intersection_7(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression="D = B {overlapped,|+} C", basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 13) # 6 + 7
        self.assertEqual(D.metadata.get_max_max(), 13) # 6 + 7
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_8(self):
        """Simple temporal intersection test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A {during,=+} buff_t(C, "1 day") ',
                  basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 8)  # 1 + 7  a1 + c1
        self.assertEqual(D.metadata.get_max_max(), 11) # 4 + 7  a4 + c1
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_neighbors_1(self):
        """Simple temporal neighborhood computation test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A[-1] + A[1]',
                  basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_neighbors_2(self):
        """Simple temporal neighborhood computation test"""
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A[0,0,-1] + A[0,0,1]',
                  basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="strds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def tearDown(self):
        ret = grass.script.run_command("t.remove", flags="rf", input="D", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        ret = grass.script.run_command("t.remove", flags="rf", input="A,B,C", quiet=True)
        grass.script.del_temp_region()

if __name__ == '__main__':
    unittest.main()


