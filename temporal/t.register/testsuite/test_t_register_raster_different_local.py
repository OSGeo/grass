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

import os
from datetime import datetime

import grass.script as gs
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestRegisterDifferentLocal(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""

        # Check UnicodeDecodeError
        os.environ["LANG"] = "fr_BE"
        os.environ["LANGUAGE"] = "fr_BE"
        os.environ["LC_CTYPE"] = "fr_BE.UTF-8"
        os.environ["LC_NUMERIC"] = "C"
        os.environ["LC_TIME"] = "fr_BE.UTF-8"
        os.environ["LC_COLLATE"] = "fr_BE.UTF-8"
        os.environ["LC_MONETARY"] = "fr_BE.UTF-8"
        os.environ["LC_MESSAGES"] = "fr_BE.UTF-8"
        os.environ["LC_PAPER"] = "fr_BE.UTF-8"
        os.environ["LC_NAME"] = "fr_BE.UTF-8"
        os.environ["LC_ADDRESS"] = "fr_BE.UTF-8"
        os.environ["LC_TELEPHONE"] = "fr_BE.UTF-8"
        os.environ["LC_MEASUREMENT"] = "fr_BE.UTF-8"
        os.environ["LC_IDENTIFICATION"] = "fr_BE.UTF-8"
        os.environ["LC_ALL"] = ""

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
                "r.support",
                map=f"prec_{raster_map_id + 1}",
                # Note: Non-ascii characters in semantic labels lead to decode error
                # semantic_label="Précipitation_totale",
                semantic_label="semantic_label",
            )

        cls.runModule(
            "t.create",
            output="pluies_nc",
            semantictype="sum",
            title="precipitation_mois",
            description="Précipitation totale mensuelle NC",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule("t.remove", flags="df", type="strds", inputs="pluies_nc")
        cls.del_temp_region()

    def test_with_different_local(self):
        register_module = SimpleModule(
            "t.register",
            flags="i",
            input="pluies_nc",
            type="raster",
            start="2000-01-01",
            increment="1 months",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
        )
        # t.info pluies_nc
        # t.topology pluies_nc
        # t.remove -f pluies_nc
        self.assertModule(register_module)

        strds = tgis.open_old_stds("pluies_nc", type="strds")

        self.assertEqual(strds.metadata.get_number_of_maps(), 6)
        start, end = strds.get_absolute_time()
        self.assertEqual(start, datetime(2000, 1, 1))
        self.assertEqual(end, datetime(2000, 7, 1))
        self.assertEqual(strds.check_temporal_topology(), True)
        self.assertEqual(strds.get_granularity(), "1 month")

        print(gs.read_command("t.info", type="strds", input="pluies_nc"))
        t_rast_list = SimpleModule("t.rast.list", input="pluies_nc")
        t_rast_list.run()

        # Check registered raster maps
        ref_str = """name|mapset|start_time|end_time
prec_1|...|2000-01-01 00:00:00|2000-02-01 00:00:00
prec_2|...|2000-02-01 00:00:00|2000-03-01 00:00:00
prec_3|...|2000-03-01 00:00:00|2000-04-01 00:00:00
prec_4|...|2000-04-01 00:00:00|2000-05-01 00:00:00
prec_5|...|2000-05-01 00:00:00|2000-06-01 00:00:00
prec_6|...|2000-06-01 00:00:00|2000-07-01 00:00:00"""
        self.assertLooksLike(str(t_rast_list.outputs.stdout), ref_str)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
