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

    def test_temporal_conditional_time_dimension_bug(self):
        """Testing the conditional time dimension bug, that uses the time
        dimension of the conditional statement instead the time dimension
        of the then/else statement."""
        self.assertModule(
            "t.rast.algebra",
            expression="R = if({contains}, B == 5, A - 1,  A + 1)",
            basename="r",
            flags="d",
        )
        self.assertModule(
            "t.rast.algebra",
            expression="R = if({contains}, B == 5, A - 1,  A + 1)",
            basename="r",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 0)  # 1 - 1
        self.assertEqual(result_strds.metadata.get_max_max(), 5)  # 4 + 1
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_intersection_1(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {+,equal,i} B", basename="r"
        )
        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 0)

    def test_temporal_intersection_2(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {+,during,i} B", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 6)  # 1 + 5
        self.assertEqual(result_strds.metadata.get_max_max(), 10)  # 4 + 6
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_3(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {+,starts,i} B", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 6)  # 1 + 5
        self.assertEqual(result_strds.metadata.get_max_max(), 9)  # 3 + 6
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_intersection_4(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {+,finishes,intersect} B", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 7)  # 2 + 5
        self.assertEqual(result_strds.metadata.get_max_max(), 10)  # 4 + 6
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_5(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {+,starts|finishes,i} B", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 6)  # 1 + 5
        self.assertEqual(result_strds.metadata.get_max_max(), 10)  # 4 + 6
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_6(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra", expression="R = B {+,overlaps,u} C", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 12)  # 5 + 7
        self.assertEqual(result_strds.metadata.get_max_max(), 12)  # 5 + 7
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_intersection_7(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra", expression="R = B {+,overlapped,u} C", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 13)  # 6 + 7
        self.assertEqual(result_strds.metadata.get_max_max(), 13)  # 6 + 7
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_intersection_8(self):
        """Simple temporal intersection test"""

        self.assertModule(
            "t.rast.algebra",
            expression='R = A {+,during,l} buff_t(C, "1 day") ',
            basename="r",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 8)  # 1 + 7  a1 + c1
        self.assertEqual(result_strds.metadata.get_max_max(), 11)  # 4 + 7  a4 + c1
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))

    def test_temporal_neighbors_1(self):
        """Simple temporal neighborhood computation test"""

        self.assertModule("t.rast.algebra", expression="R = A[-1] + A[1]", basename="r")

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(result_strds.metadata.get_max_max(), 6)  # 2 + 4
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_neighbors_2(self):
        """Simple temporal neighborhood computation test"""

        self.assertModule(
            "t.rast.algebra", expression="R = A[0,0,-1] + A[0,0,1]", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(result_strds.metadata.get_max_max(), 6)  # 2 + 4
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_tmap_function1(self):
        """Testing the tmap function."""

        self.assertModule(
            "t.rast.algebra", expression="R = tmap(singletmap)", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 99)
        self.assertEqual(result_strds.metadata.get_max_max(), 99)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_tmap_function2(self):
        """Testing the tmap function."""

        self.assertModule(
            "t.rast.algebra", expression="R = tmap(singletmap) + 1", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 100)
        self.assertEqual(result_strds.metadata.get_max_max(), 100)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_one_strds(self):
        """Testing the temporal select operator with one STRDS."""

        self.assertModule("t.rast.algebra", expression="R = A : A", basename="r")

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_two_strds(self):
        """Testing the temporal select operator with two STRDS."""

        self.assertModule("t.rast.algebra", expression="R = A : D", basename="r")

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 3)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_operators1(self):
        """Testing the temporal select operator. Including temporal relations."""

        self.assertModule("t.rast.algebra", expression="R = A : D", basename="r")

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 3)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_operators2(self):
        """Testing the temporal select operator. Including temporal relations."""

        self.assertModule(
            "t.rast.algebra", expression="R = A {!:,during} C", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_operators3(self):
        """Testing the temporal select operator. Including temporal relations and
        different temporal operators (lr|+&)"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {:,during,d} B", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), False)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_temporal_select_operators4(self):
        """Testing the temporal select operator. Including temporal relations and
        different temporal operators (lr|+&)"""

        self.assertModule(
            "t.rast.algebra", expression="R = A {:,equal|during,r} C", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        map_list = result_strds.get_registered_maps_as_objects()
        for map_i in map_list:
            start_map, end_map = map_i.get_absolute_time()
            self.assertEqual(start_map, datetime.datetime(2001, 1, 2))
            self.assertEqual(end_map, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), False)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_temporal_hash_operator1(self):
        """Testing the temporal hash operator in the raster algebra."""

        self.assertModule(
            "t.rast.algebra", expression="R = if(A # D == 1, A)", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 3)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_hash_operator2(self):
        """Testing the temporal hash operator in the raster algebra."""

        self.assertModule("t.rast.algebra", expression="R = A # D", basename="r")

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 1)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_hash_operator3(self):
        """Testing the temporal hash operator in the raster algebra."""

        self.assertModule(
            "t.rast.algebra", expression="R = C {#,contains} A", basename="r"
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 2)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_temporal_hash_operator4(self):
        """Testing the temporal hash operator in the raster algebra."""

        self.assertModule(
            "t.rast.algebra",
            expression="R = if({contains},A # D == 1, C {#,contains} A)",
            basename="r",
        )

        result_strds = tgis.open_old_stds("R", type="strds")

        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 2)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "2 days")


if __name__ == "__main__":
    test()
