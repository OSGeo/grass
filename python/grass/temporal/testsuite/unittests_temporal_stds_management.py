"""Unit tests for shift() and snap() methods
of AbstractSpaceTimeDataset

(C) 2026 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Selma Bentaiba
"""

import datetime
import os
import uuid

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.temporal as tgis


class TestShift(TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        os.putenv("GRASS_OVERWRITE", "1")
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule(
            "g.region", n=80.0, s=0.0, e=120.0, w=0.0, t=1.0, b=0.0, res=10.0
        )

    @classmethod
    def tearDownClass(cls) -> None:
        cls.del_temp_region()

    def setUp(self) -> None:
        uid = uuid.uuid4().hex[:6]
        self.map1 = f"map_1_{uid}"
        self.map2 = f"map_2_{uid}"
        self.strds_name = f"shift_test_{uid}"
        self.runModule(
            "r.mapcalc", overwrite=True, quiet=True,
            expression=f"{self.map1} = 1"
        )
        self.runModule(
            "r.mapcalc", overwrite=True, quiet=True,
            expression=f"{self.map2} = 2"
        )
        self.strds = tgis.open_new_stds(
            name=self.strds_name,
            type="strds",
            temporaltype="absolute",
            title="Test shift",
            descr="Test shift",
            semantic="field",
            overwrite=True,
        )
        tgis.register_maps_in_space_time_dataset(
            type="raster",
            name=self.strds.get_name(),
            maps=f"{self.map1},{self.map2}",
            start="2001-01-01",
            increment="1 day",
            interval=True,
        )

    def tearDown(self) -> None:
        self.runModule(
            "t.unregister",
            type="raster",
            maps=f"{self.map1},{self.map2}",
            quiet=True,
        )
        self.runModule(
            "g.remove", flags="f", type="raster",
            name=f"{self.map1},{self.map2}", quiet=True
        )
        self.strds.delete()

    def test_shift_absolute_time(self) -> None:
        """shift() moves all map timestamps forward by the given granularity"""
        self.strds.shift(gran="1 days")

        maps = self.strds.get_registered_maps_as_objects(
            where=None, order="start_time"
        )
        start, end = maps[0].get_temporal_extent_as_tuple()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        start, end = maps[1].get_temporal_extent_as_tuple()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_shift_invalid_granularity(self) -> None:
        """shift() returns False for invalid granularity string"""
        result = self.strds.shift(gran="invalid")
        self.assertFalse(result)

# TODO: test_shift_relative_time is disabled pending GRASS 8.x testing.
    # The registration of relative time maps with unit="seconds" fails in
    # GRASS 7.8.7 with "Unsupported relative time unit type: None".
    # This should be re-enabled and verified once tested against GRASS 8.x.
    # Related to #7231.
    #
    # def test_shift_relative_time(self) -> None:
    #     """shift() moves timestamps forward for relative time datasets"""
    #     uid = uuid.uuid4().hex[:6]
    #     map_rel_1 = f"map_rel_1_{uid}"
    #     map_rel_2 = f"map_rel_2_{uid}"
    #     strds_rel_name = f"shift_test_rel_{uid}"
    #     self.runModule(
    #         "r.mapcalc", overwrite=True, quiet=True,
    #         expression=f"{map_rel_1} = 1"
    #     )
    #     self.runModule(
    #         "r.mapcalc", overwrite=True, quiet=True,
    #         expression=f"{map_rel_2} = 2"
    #     )
    #     strds_rel = tgis.open_new_stds(
    #         name=strds_rel_name,
    #         type="strds",
    #         temporaltype="relative",
    #         title="Test shift relative",
    #         descr="Test shift relative",
    #         semantic="field",
    #         overwrite=True,
    #     )
    #     tgis.register_maps_in_space_time_dataset(
    #         type="raster",
    #         name=strds_rel.get_name(),
    #         maps=f"{map_rel_1},{map_rel_2}",
    #         start=0,
    #         increment=1,
    #         unit="seconds",
    #         interval=True,
    #     )
    #     strds_rel.shift(gran="5")
    #     maps = strds_rel.get_registered_maps_as_objects(
    #         where=None, order="start_time"
    #     )
    #     start, end = maps[0].get_temporal_extent_as_tuple()
    #     self.assertEqual(start, 5)
    #     self.assertEqual(end, 6)
    #     self.runModule(
    #         "t.unregister",
    #         type="raster",
    #         maps=f"{map_rel_1},{map_rel_2}",
    #         quiet=True,
    #     )
    #     strds_rel.delete()
    #     self.runModule(
    #         "g.remove", flags="f", type="raster",
    #         name=f"{map_rel_1},{map_rel_2}", quiet=True
    #     )

class TestSnap(TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        os.putenv("GRASS_OVERWRITE", "1")
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule(
            "g.region", n=80.0, s=0.0, e=120.0, w=0.0, t=1.0, b=0.0, res=10.0
        )

    @classmethod
    def tearDownClass(cls) -> None:
        cls.del_temp_region()

    def setUp(self) -> None:
        uid = uuid.uuid4().hex[:6]
        self.map1 = f"map_1_{uid}"
        self.map2 = f"map_2_{uid}"
        self.strds_name = f"snap_test_{uid}"
        self.runModule(
            "r.mapcalc", overwrite=True, quiet=True,
            expression=f"{self.map1} = 1"
        )
        self.runModule(
            "r.mapcalc", overwrite=True, quiet=True,
            expression=f"{self.map2} = 2"
        )
        self.strds = tgis.open_new_stds(
            name=self.strds_name,
            type="strds",
            temporaltype="absolute",
            title="Test snap",
            descr="Test snap",
            semantic="field",
            overwrite=True,
        )
        tgis.register_maps_in_space_time_dataset(
            type="raster",
            name=self.strds.get_name(),
            maps=f"{self.map1},{self.map2}",
            start="2001-01-01",
            increment="2 days",
            interval=False,
        )

    def tearDown(self) -> None:
        self.runModule(
            "t.unregister",
            type="raster",
            maps=f"{self.map1},{self.map2}",
            quiet=True,
        )
        self.runModule(
            "g.remove", flags="f", type="raster",
            name=f"{self.map1},{self.map2}", quiet=True
        )
        self.strds.delete()

    def test_snap_closes_gaps(self) -> None:
        """snap() sets end time of each map to start time of next neighbor"""
        self.strds.snap()

        maps = self.strds.get_registered_maps_as_objects(
            where=None, order="start_time"
        )
        start, end = maps[0].get_temporal_extent_as_tuple()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))


if __name__ == "__main__":
    test()
