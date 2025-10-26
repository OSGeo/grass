"""Test t.register

(C) 2014-2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
        Ported to Python by Stefan Blumentrath

This is a test to register and unregister raster maps in
space time raster input.
The raster maps will be registered in different space time raster
inputs

We need to set a specific region in the
@preprocess step of this test. We generate
raster with r.mapcalc and create two space time raster inputs
with absolute time
The region setting should work for UTM and LL test locations

"""

from datetime import datetime

import grass.script as gs
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestRegister(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""

        tgis.init()
        # cls.mapset = gs.gisenv()["MAPSET"]
        cls.use_temp_region()
        cls.runModule(
            "g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10, flags="p3"
        )

        # Generate data
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_1 = rand(0, 550)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_2 = rand(0, 450)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_3 = rand(0, 320)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_4 = rand(0, 510)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_5 = rand(0, 300)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_6 = rand(0, 650)", overwrite=True
        )

        for strds_id in range(1, 9):
            cls.runModule(
                "t.create",
                type="strds",
                temporaltype="absolute",
                output=f"precip_abs{strds_id}",
                title="A test",
                description="A test",
                overwrite=True,
            )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        stds = gs.read_command("t.list")
        for strds_id in range(1, 9):
            if f"precip_abs{strds_id}" in stds:
                cls.runModule(
                    "t.remove", flags="df", type="strds", inputs=f"precip_abs{strds_id}"
                )
        cls.del_temp_region()

    def test_with_second_increment_and_intervall(self):
        register_module = SimpleModule(
            "t.register",
            flags="i",
            input="precip_abs1",
            start="2001-01-01",
            increment="1 seconds",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs1", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1, 0, 0, 0))
        self.assertEqual(end, datetime(2001, 1, 1, 0, 0, 6))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "1 second")

        print(gs.read_command("t.info", type="strds", input="precip_abs1"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs1")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|2001-01-01 00:00:01
prec_2|...|2001-01-01 00:00:01|2001-01-01 00:00:02
prec_3|...|2001-01-01 00:00:02|2001-01-01 00:00:03
prec_4|...|2001-01-01 00:00:03|2001-01-01 00:00:04
prec_5|...|2001-01-01 00:00:04|2001-01-01 00:00:05
prec_6|...|2001-01-01 00:00:05|2001-01-01 00:00:06"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

    def test_with_minutes_seconds_increment_and_intervall(self):
        register_module = SimpleModule(
            "t.register",
            flags="i",
            input="precip_abs2",
            start="2001-01-01",
            increment="20 seconds, 5 minutes",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs2", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2001, 1, 1, 0, 32))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "320 seconds")

        print(gs.read_command("t.info", type="strds", input="precip_abs2"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs2")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|2001-01-01 00:05:20
prec_2|...|2001-01-01 00:05:20|2001-01-01 00:10:40
prec_3|...|2001-01-01 00:10:40|2001-01-01 00:16:00
prec_4|...|2001-01-01 00:16:00|2001-01-01 00:21:20
prec_5|...|2001-01-01 00:21:20|2001-01-01 00:26:40
prec_6|...|2001-01-01 00:26:40|2001-01-01 00:32:00"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

    def test_with_hours_increment_and_intervall(self):
        register_module = SimpleModule(
            "t.register",
            flags="i",
            input="precip_abs3",
            start="2001-01-01",
            increment="8 hours",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        print(gs.read_command("t.info", type="strds", input="precip_abs3"))
        strds = tgis.open_old_stds("precip_abs3", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2001, 1, 3))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "8 hours")

        print(gs.read_command("t.info", type="strds", input="precip_abs3"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs3")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|2001-01-01 08:00:00
prec_2|...|2001-01-01 08:00:00|2001-01-01 16:00:00
prec_3|...|2001-01-01 16:00:00|2001-01-02 00:00:00
prec_4|...|2001-01-02 00:00:00|2001-01-02 08:00:00
prec_5|...|2001-01-02 08:00:00|2001-01-02 16:00:00
prec_6|...|2001-01-02 16:00:00|2001-01-03 00:00:00"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

    def test_with_days_increment_and_no_intervall(self):
        register_module = SimpleModule(
            "t.register",
            input="precip_abs4",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 days",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs4", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2001, 1, 16))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "3 days")

        print(gs.read_command("t.info", type="strds", input="precip_abs4"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs4")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|None
prec_2|...|2001-01-04 00:00:00|None
prec_3|...|2001-01-07 00:00:00|None
prec_4|...|2001-01-10 00:00:00|None
prec_5|...|2001-01-13 00:00:00|None
prec_6|...|2001-01-16 00:00:00|None"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

    def test_with_weeks_increment_and_no_intervall(self):
        register_module = SimpleModule(
            "t.register",
            input="precip_abs5",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="4 weeks",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs5", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2001, 5, 21))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "28 days")

        print(gs.read_command("t.info", type="strds", input="precip_abs5"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs5")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|None
prec_2|...|2001-01-29 00:00:00|None
prec_3|...|2001-02-26 00:00:00|None
prec_4|...|2001-03-26 00:00:00|None
prec_5|...|2001-04-23 00:00:00|None
prec_6|...|2001-05-21 00:00:00|None"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

    def test_with_months_increment_and_no_intervall(self):
        register_module = SimpleModule(
            "t.register",
            input="precip_abs6",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-08-01",
            increment="2 months",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs6", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 8, 1))
        self.assertEqual(end, datetime(2002, 6, 1))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "2 months")

        print(gs.read_command("t.info", type="strds", input="precip_abs6"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs6")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-08-01 00:00:00|None
prec_2|...|2001-10-01 00:00:00|None
prec_3|...|2001-12-01 00:00:00|None
prec_4|...|2002-02-01 00:00:00|None
prec_5|...|2002-04-01 00:00:00|None
prec_6|...|2002-06-01 00:00:00|None"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

    def test_re_registering(self):
        # t.register  input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="20 years, 3 months, 1 days, 4 hours"
        # t.register  input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
        # t.register  -i input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
        # t.register  input=precip_abs7 maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6 start="2001-01-01" end="2002-01-01"
        register_module = SimpleModule(
            "t.register",
            input="precip_abs7",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="20 years, 3 months, 1 days, 4 hours",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs7", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2102, 4, 6, 20, 0))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "4 hours")

        print(gs.read_command("t.info", type="strds", input="precip_abs7"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs7")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|None
prec_2|...|2021-04-02 04:00:00|None
prec_3|...|2041-07-03 08:00:00|None
prec_4|...|2061-10-04 12:00:00|None
prec_5|...|2082-01-05 16:00:00|None
prec_6|...|2102-04-06 20:00:00|None"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

        # Register with different valid time again
        register_module = SimpleModule(
            "t.register",
            input="precip_abs7",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="99 years, 9 months, 9 days, 9 hours",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs7", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2499, 11, 16, 21, 0))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "3 hours")

        # Check raster map metadata (time stamp)
        self.assertRasterFitsInfo(
            raster="prec_3",
            reference='timestamp="19 Jul 2200 18:00:00"',
        )

        print(gs.read_command("t.info", type="strds", input="precip_abs7"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs7")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|None
prec_2|...|2100-10-10 09:00:00|None
prec_3|...|2200-07-19 18:00:00|None
prec_4|...|2300-04-29 03:00:00|None
prec_5|...|2400-02-07 12:00:00|None
prec_6|...|2499-11-16 21:00:00|None"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)

        # Register with different valid time again creating an interval
        register_module = SimpleModule(
            "t.register",
            flags="i",
            input="precip_abs7",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="99 years, 9 months, 9 days, 9 hours",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        strds = tgis.open_old_stds("precip_abs7", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2599, 8, 26, 6, 0))
        self.assertEqual(strds.check_temporal_topology(), False)
        self.assertEqual(strds.get_granularity(), "3 hours")

        # Check raster map metadata (time stamp)
        self.assertRasterFitsInfo(
            raster="prec_3",
            reference='timestamp="19 Jul 2200 18:00:00 / 29 Apr 2300 03:00:00"',
        )

        print(gs.read_command("t.info", type="strds", input="precip_abs7"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs7")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2001-01-01 00:00:00|2100-10-10 09:00:00
prec_2|...|2100-10-10 09:00:00|2200-07-19 18:00:00
prec_3|...|2200-07-19 18:00:00|2300-04-29 03:00:00
prec_4|...|2300-04-29 03:00:00|2400-02-07 12:00:00
prec_5|...|2400-02-07 12:00:00|2499-11-17 21:00:00
prec_6|...|2499-11-16 21:00:00|2599-08-26 06:00:00"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
