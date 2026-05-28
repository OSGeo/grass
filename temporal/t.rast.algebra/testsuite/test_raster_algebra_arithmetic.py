"""
(C) 2013-2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import datetime
import os

import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.main import test


class TestTRastAlgebra(TestCase):
    """Class for testing t.rast.algebra"""

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        os.putenv("GRASS_OVERWRITE", "1")
        tgis.init(True)  # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0, w=0.0, t=1.0, b=0.0, res=10.0)

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

        tgis.open_new_stds(
            name="A",
            type="strds",
            temporaltype="absolute",
            title="A",
            descr="A",
            semantic="field",
        )
        tgis.open_new_stds(
            name="B",
            type="strds",
            temporaltype="absolute",
            title="B",
            descr="B",
            semantic="field",
        )
        tgis.open_new_stds(
            name="C",
            type="strds",
            temporaltype="absolute",
            title="B",
            descr="C",
            semantic="field",
        )
        tgis.open_new_stds(
            name="D",
            type="strds",
            temporaltype="absolute",
            title="D",
            descr="D",
            semantic="field",
        )

        tgis.register_maps_in_space_time_dataset(
            type="raster",
            name="A",
            maps="a1,a2,a3,a4",
            start="2001-01-01",
            increment="1 day",
            interval=True,
        )
        tgis.register_maps_in_space_time_dataset(
            type="raster",
            name="B",
            maps="b1,b2",
            start="2001-01-01",
            increment="2 day",
            interval=True,
        )
        tgis.register_maps_in_space_time_dataset(
            type="raster",
            name="C",
            maps="c1",
            start="2001-01-02",
            increment="2 day",
            interval=True,
        )
        tgis.register_maps_in_space_time_dataset(
            type="raster",
            name="D",
            maps="d1,d2,d3",
            start="2001-01-03",
            increment="1 day",
            interval=True,
        )
        tgis.register_maps_in_space_time_dataset(
            type="raster",
            name=None,
            maps="singletmap",
            start="2001-01-03",
            end="2001-01-04",
        )

    def tearDown(self):
        try:
            self.runModule("t.remove", flags="df", inputs="R", quiet=True)
        except Exception:
            # STRDS "R" has not been created
            pass

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule("t.remove", flags="df", inputs="A,B,C,D", quiet=True)
        cls.runModule("t.unregister", maps="singletmap", quiet=True)
        cls.del_temp_region()

    def test_simple_arithmetic_hash_1(self):
        """Simple arithmetic test including the hash operator"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = A + (A {#, equal,l} A)",
            basename="r",
            flags="d",
        )
        self.assertModule(
            "t.rast.algebra", expression="R = A + (A {#, equal,l} A)", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 5)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arithmetic_td_1(self):
        """Simple arithmetic test"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = A + td(A)",
            basename="r",
            flags="d",
            suffix="time",
        )
        self.assertModule(
            "t.rast.algebra", expression="R = A + td(A)", basename="r", suffix="time"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 5)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arithmetic_td_2(self):
        """Simple arithmetic test"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = A / td(A)",
            basename="r",
            flags="d",
            suffix="gran",
        )
        self.assertModule(
            "t.rast.algebra", expression="R = A / td(A)", basename="r", suffix="gran"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arithmetic_td_3(self):
        """Simple arithmetic test"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = A {+,equal} td(A)",
            basename="r",
            flags="d",
        )
        self.assertModule(
            "t.rast.algebra", expression="R = A {+,equal} td(A)", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 5)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arithmetic_td_4(self):
        """Simple arithmetic test"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = A {/, equal} td(A)",
            basename="r",
            flags="d",
        )
        self.assertModule(
            "t.rast.algebra", expression="R = A {/, equal} td(A)", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arithmetic_time_function_expressions(self):
        """Simple arithmetic to test the generated expression of a time function"""

        print_module_run = SimpleModule(
            "t.rast.algebra",
            expression="R = if(A >= 3, start_doy(A, 0), 0)",
            basename="r",
            flags="d",
            suffix="gran",
        )
        print_module_run.run()

        # Check expressions
        ref_str = "...r_2001_01_01=if(a1@...>=3,1,0)...r_2001_01_02=if(a2@...>=3,2,0)...r_2001_01_03=if(a3@...>=3,3,0)...r_2001_01_04=if(a4@...>=3,4,0)..."
        print(str(print_module_run.outputs.stdout.replace("\n", "").replace(" ", "")))
        self.assertLooksLike(
            str(print_module_run.outputs.stdout.replace("\n", "").replace(" ", "")),
            ref_str,
        )

    def test_simple_arithmetic_time_function_results(self):
        """Simple arithmetic to test the results of a time function"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = if(A >= 3, start_doy(A, 0), 0)",
            basename="r",
            suffix="gran",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 0)
        self.assertEqual(result_strds.metadata.get_min_max(), 4)
        self.assertEqual(result_strds.metadata.get_max_min(), 0)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

        # Check results around if-threshold
        self.assertRasterMinMax("r_2001_01_02", 0, 0)
        self.assertRasterMinMax("r_2001_01_03", 3, 3)

    def test_simple_arithmetic_if_1(self):
        """Simple arithmetic test with if condition"""

        self.assertModule(
            "t.rast.algebra",
            expression='R = if({equal}, start_date(A) >= "2001-01-02", A + A)',
            basename="r",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 3)
        self.assertEqual(result_strds.metadata.get_min_min(), 4)
        self.assertEqual(result_strds.metadata.get_max_max(), 8)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arithmetic_if_2(self):
        """Simple arithmetic test with if condition"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = if({equal}, A#A == 1, A - A)",
            basename="r",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 0)
        self.assertEqual(result_strds.metadata.get_max_max(), 0)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_complex_arithmetic_if_1(self):
        """Complex arithmetic test with if condition"""

        self.assertModule(
            "t.rast.algebra",
            expression='R = if(start_date(A) < "2001-01-03" && A#A == 1,'
            " A{+, starts,l}C, A{+, finishes,l}C)",
            basename="r",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 9)  # 2 + 7 a2 + c1
        self.assertEqual(result_strds.metadata.get_max_max(), 10)  # 3 + 7 a3 + c1
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_simple_arithmetic_1(self):
        """Simple arithmetic test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {*, equal} A {+, equal} A", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)  # 1*1 + 1
        self.assertEqual(result_strds.metadata.get_max_max(), 20)  # 4*4 + 4
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_simple_arithmetic_2(self):
        """Simple arithmetic test that creates an empty strds"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = A {*, during} A {+, during} A",
            basename="r",
        )
        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 0)

    def test_simple_arithmetic_3(self):
        """Simple arithmetic test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A / A + A*A/A", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)  # 1/1 + 1*1/1
        self.assertEqual(result_strds.metadata.get_max_max(), 5)  # 4/4 + 4*4/4
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_map_function1(self):
        """Testing the map function."""

        self.assertModule(
            "t.rast.algebra", expression="R = map(singlemap) + A", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 101)
        self.assertEqual(result_strds.metadata.get_max_max(), 104)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_map_function2(self):
        """Testing the map function."""

        self.assertModule(
            "t.rast.algebra", expression="R =  A * map(singlemap)", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 100)
        self.assertEqual(result_strds.metadata.get_max_max(), 400)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_raster_arithmetic_relation_1(self):
        """Arithmetic test with temporal intersection"""

        self.assertModule(
            "t.rast.algebra", expression="R = B {+,contains,l} A ", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 8)
        self.assertEqual(result_strds.metadata.get_max_max(), 13)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_raster_arithmetic_relation_2(self):
        """Arithmetic test with temporal intersection"""

        self.assertModule(
            "t.rast.algebra", expression="R = B {*,contains,l} A ", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 10)
        self.assertEqual(result_strds.metadata.get_max_max(), 72)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_raster_arithmetic_relation_3(self):
        """Arithmetic test with temporal intersection"""

        self.assertModule(
            "t.rast.algebra", expression="R = B {+,contains,l} A ", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 8)
        self.assertEqual(result_strds.metadata.get_max_max(), 13)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_raster_arithmetic_relation_4(self):
        """Arithmetic test with temporal intersection"""

        self.assertModule(
            "t.rast.algebra", expression="R = B {+,contains,r} A ", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 8)
        self.assertEqual(result_strds.metadata.get_max_max(), 13)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_raster_arithmetic_relation_5(self):
        """Complex arithmetic test with temporal intersection"""

        self.assertModule(
            "t.rast.algebra",
            expression="R = tmap(singletmap) "
            "{+,equal| precedes| follows,l} "
            "A + map(singlemap)",
            basename="r",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 208)
        self.assertEqual(result_strds.metadata.get_max_max(), 208)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")


if __name__ == "__main__":
    test()
