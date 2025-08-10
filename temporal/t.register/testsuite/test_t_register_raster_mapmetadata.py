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
from grass.temporal.datetime_math import (
    datetime_to_grass_datetime_string as datetime_to_grass,
)


class TestRegisterMapMetadata(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""

        tgis.init()
        # cls.mapset = gs.gisenv()["MAPSET"]
        cls.use_temp_region()
        cls.runModule(
            "g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10, flags="p3"
        )

        for raster_map_id, parameters in enumerate(
            [
                (550, "2001-01-01 00:00:00|2001-01-01 08:00:00"),
                (450, "2001-01-01 08:00:00|2001-01-01 16:00:00"),
                (320, "2001-01-01 16:00:00|2001-01-02 00:00:00"),
                (510, "2001-01-02 00:00:00|2001-01-02 08:00:00"),
                (300, "2001-01-02 08:00:00|2001-01-02 16:00:00"),
                (650, "2001-01-02 16:00:00|2001-01-03 00:00:00"),
            ]
        ):
            # Generate data
            cls.runModule(
                "r.mapcalc",
                flags="s",
                expression=f"prec_{raster_map_id + 1} = rand(0, {parameters[0]})",
                overwrite=True,
            )
            cls.runModule(
                "r.timestamp",
                map=f"prec_{raster_map_id + 1}",
                date="/".join(
                    [
                        datetime_to_grass(datetime.fromisoformat(ts))
                        for ts in parameters[1].split("|")
                    ]
                ),
            )
            cls.runModule(
                "r.support",
                map=f"prec_{raster_map_id + 1}",
                semantic_label="semantic_label",
            )

        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule("t.remove", flags="df", type="strds", inputs="precip_abs1")
        cls.del_temp_region()

    def test_register_metadata_from_file(self):
        register_module = SimpleModule(
            "t.register",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        print(gs.read_command("t.info", type="strds", input="precip_abs1"))
        strds = tgis.open_old_stds("precip_abs1", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2001, 1, 1))
        self.assertEqual(end, datetime(2001, 1, 3))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "8 hours")
        self.assertEqual(strds.metadata.get_number_of_semantic_labels(), 1)
        self.assertEqual(strds.metadata.get_semantic_labels(), "semantic_label")

        print(gs.read_command("t.info", type="strds", input="precip_abs1"))
        t_rast_list = SimpleModule("t.rast.list", input="precip_abs1")
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


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
