"""
(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""

import datetime

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.temporal as tgis


class TestTemporalAlgebra(TestCase):
    """Class for testing temporal algebra"""

    @classmethod
    def setUpClass(cls) -> None:
        """Initiate the temporal GIS and set the region"""
        tgis.init(True)  # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0, w=0.0, t=1.0, b=0.0, res=10.0)

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
        cls.runModule(
            "r.mapcalc", overwrite=True, quiet=True, expression="singletmap = 99"
        )

        tgis.open_new_stds(
            name="A",
            type="strds",
            temporaltype="absolute",
            title="A",
            descr="A",
            semantic="field",
            overwrite=True,
        )
        tgis.open_new_stds(
            name="B",
            type="strds",
            temporaltype="absolute",
            title="B",
            descr="B",
            semantic="field",
            overwrite=True,
        )
        tgis.open_new_stds(
            name="C",
            type="strds",
            temporaltype="absolute",
            title="C",
            descr="C",
            semantic="field",
            overwrite=True,
        )
        tgis.open_new_stds(
            name="D",
            type="strds",
            temporaltype="absolute",
            title="D",
            descr="D",
            semantic="field",
            overwrite=True,
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

    def tearDown(self) -> None:
        self.runModule("t.remove", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls) -> None:
        """Remove the temporary region"""
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", quiet=True)
        cls.del_temp_region()

    def test_temporal_select1(self) -> None:
        """Testing the temporal select operator with equal relations."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A : A", stdstype="strds", basename="r", overwrite=True
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select2(self) -> None:
        """Testing the temporal select operator with equal relations."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A : D", stdstype="strds", basename="r", overwrite=True
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 3)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select3(self) -> None:
        """Testing the temporal select operator with equal relations."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A !: D", stdstype="strds", basename="r", overwrite=True
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 2)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_operators1(self) -> None:
        """Testing the temporal select operator. Including temporal relations."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A {:,during} C",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_operators2(self) -> None:
        """Testing the temporal select operator. Including temporal relations."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A {:,equal|during} C",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_operators3(self) -> None:
        """Testing the temporal select operator. Including temporal relations
        and negation operation."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A {!:,during} C",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_select_operators4(self) -> None:
        """Testing the temporal select operator. Including temporal relations and
        temporal operators."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A {:,during,d} C",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
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

    def test_temporal_select_operators5(self) -> None:
        """Testing the temporal select operator. Including temporal relations and
        temporal operators."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = C {:,contains} A",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        map_list = result_strds.get_registered_maps_as_objects()
        for map_i in map_list:
            start_map, end_map = map_i.get_absolute_time()
            self.assertEqual(start_map, datetime.datetime(2001, 1, 2))
            self.assertEqual(end_map, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 7)
        self.assertEqual(result_strds.metadata.get_max_max(), 7)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_temporal_extent1(self) -> None:
        """Testing the temporal extent operators."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A {:,during,r} C",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), False)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_temporal_extent2(self) -> None:
        """Testing the temporal extent operators."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A {:,during,d} C",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), False)
        self.assertEqual(result_strds.get_granularity(), "2 days")

    def test_temporal_extent3(self) -> None:
        """Testing the temporal extent operators."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A {:,during,u} C",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
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

    def test_temporal_hash1(self) -> None:
        """Testing the hash function in conditional statement."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = if(A # D == 1, A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 3)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_hash_operator1(self) -> None:
        """Testing the hash operator function in conditional statement."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = if(A {#,during} C == 1, A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_temporal_hash_operator2(self) -> None:
        """Testing the hash operator function in conditional statement."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = if({during}, C {#,contains} A == 2, A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 2)
        self.assertEqual(result_strds.metadata.get_min_min(), 2)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_tmap_function1(self) -> None:
        """Testing the tmap function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = tmap(singletmap)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 99)
        self.assertEqual(result_strds.metadata.get_max_max(), 99)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_tmap_function2(self) -> None:
        """Testing the tmap function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = A : tmap(singletmap)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 1)
        self.assertEqual(result_strds.metadata.get_min_min(), 3)
        self.assertEqual(result_strds.metadata.get_max_max(), 3)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_merge_function1(self) -> None:
        """Testing the merge function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = merge(A,D)", stdstype="strds", basename="r", overwrite=True
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 7)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 10)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 6))
        self.assertEqual(result_strds.check_temporal_topology(), False)
        self.assertEqual(result_strds.get_granularity(), "1 day")

    def test_merge_function2(self) -> None:
        """Testing the merge function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(run=True, debug=True)
        temporal_algebra_parser.parse(
            expression="R = merge(A, B {!:,contains} A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        result_strds = tgis.open_old_stds("R", type="strds")
        self.assertTrue(result_strds.is_in_db())
        result_strds.select()
        self.assertEqual(result_strds.metadata.get_number_of_maps(), 4)
        self.assertEqual(result_strds.metadata.get_min_min(), 1)
        self.assertEqual(result_strds.metadata.get_max_max(), 4)
        start, end = result_strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(result_strds.check_temporal_topology(), True)
        self.assertEqual(result_strds.get_granularity(), "1 day")


class TestTemporalAlgebraDryRun(TestCase):
    """Class for testing dry runs of the temporal algebra"""

    @classmethod
    def setUpClass(cls) -> None:
        """Initiate the temporal GIS and set the region"""
        tgis.init(True)  # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0, w=0.0, t=1.0, b=0.0, res=10.0)

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
        cls.runModule(
            "r.mapcalc", overwrite=True, quiet=True, expression="singletmap = 99"
        )

        tgis.open_new_stds(
            name="A",
            type="strds",
            temporaltype="absolute",
            title="A",
            descr="A",
            semantic="field",
            overwrite=True,
        )
        tgis.open_new_stds(
            name="B",
            type="strds",
            temporaltype="absolute",
            title="B",
            descr="B",
            semantic="field",
            overwrite=True,
        )
        tgis.open_new_stds(
            name="C",
            type="strds",
            temporaltype="absolute",
            title="C",
            descr="C",
            semantic="field",
            overwrite=True,
        )
        tgis.open_new_stds(
            name="D",
            type="strds",
            temporaltype="absolute",
            title="D",
            descr="D",
            semantic="field",
            overwrite=True,
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

    @classmethod
    def tearDownClass(cls) -> None:
        """Remove the temporary region"""
        cls.runModule("t.remove", flags="rf", inputs="A,B,C,D", quiet=True)
        cls.del_temp_region()

    def test_merge_function1(self) -> None:
        """Testing the merge function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression="R = merge(A, B {:,contains} A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        self.assertEqual(len(parser_content["register"]), 6)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")

    def test_merge_function2(self) -> None:
        """Testing the merge function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression="R = merge(A, B {!:,contains} A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        self.assertEqual(len(parser_content["register"]), 4)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")

    def test_merge_function3(self) -> None:
        """Testing the merge function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression="R = merge(A, D {!:,equal} A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )

        self.assertEqual(len(parser_content["register"]), 5)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")

    def test_shift1(self) -> None:
        """Testing the shift function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression='R = tshift(A, "3 days")',
            stdstype="strds",
            basename="r",
            overwrite=True,
        )
        print(parser_content["register"])
        self.assertEqual(len(parser_content["register"]), 4)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")

    def test_shift2(self) -> None:
        """Testing the shift function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression='R = tshift(A, "2 days") {:,during,l} C',
            stdstype="strds",
            basename="r",
            overwrite=True,
        )
        print(parser_content["register"])
        self.assertEqual(len(parser_content["register"]), 1)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")

    def test_buffer1(self) -> None:
        """Testing the shift function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression='R = buff_t(A, "1 day") ',
            stdstype="strds",
            basename="r",
            overwrite=True,
        )
        print(parser_content["register"])
        self.assertEqual(len(parser_content["register"]), 4)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")

    def test_buff2(self) -> None:
        """Testing the shift function."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression='R = buff_t(A, "1 day") {:,contains,l} C',
            stdstype="strds",
            basename="r",
            overwrite=True,
        )
        print(parser_content["register"])
        self.assertEqual(len(parser_content["register"]), 2)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")

    def test_time_constant(self) -> None:
        """Testing the time constant functions."""
        temporal_algebra_parser = tgis.TemporalAlgebraParser(
            run=True, debug=False, dry_run=True
        )
        parser_content = temporal_algebra_parser.parse(
            expression="R = if(start_doy(A)<3,start_doy(A, 1), A)",
            stdstype="strds",
            basename="r",
            overwrite=True,
        )
        print(parser_content["register"])
        self.assertEqual(len(parser_content["register"]), 4)
        self.assertEqual(parser_content["STDS"]["name"], "R")
        self.assertEqual(parser_content["STDS"]["stdstype"], "strds")


if __name__ == "__main__":
    test()
