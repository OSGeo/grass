"""Test t.register
(C) 2014-2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
@author Soeren Gebbert
        Ported to Python by Stefan Blumentrath

Tests the rename module of space time datasets

We need to set a specific region in the
@preprocess step of this test. We generate
raster with r.mapcalc and create two space time raster inputs
with absolute time

The region setting should work for UTM and LL test locations
"""

import grass.script as gs
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestRenameSTDS(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""

        tgis.init()
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

        # We need to create three space time dataset
        cls.runModule(
            "t.create",
            verbose=True,
            overwrite=True,
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="Test",
            description="This is the 1 test strds",
            semantictype="sum",
        )
        cls.runModule(
            "t.register",
            flags="i",
            overwrite=True,
            input="precip_abs1",
            maps="prec_1,prec_2",
            start="2001-01-01",
            increment="1 seconds",
        )
        cls.runModule(
            "t.create",
            verbose=True,
            overwrite=True,
            type="strds",
            temporaltype="absolute",
            output="precip_abs2",
            title="Test",
            description="This is the 2 test strds",
            semantictype="sum",
        )
        cls.runModule(
            "t.register", overwrite=True, input="precip_abs2", maps="prec_1,prec_2"
        )
        cls.runModule(
            "t.create",
            verbose=True,
            overwrite=True,
            type="strds",
            temporaltype="absolute",
            output="precip_abs3",
            title="Test",
            description="This is the 3 test strds",
            semantictype="sum",
        )
        cls.runModule(
            "t.register", overwrite=True, input="precip_abs3", maps="prec_1,prec_2"
        )
        cls.runModule(
            "t.create",
            verbose=True,
            overwrite=True,
            type="strds",
            temporaltype="absolute",
            output="precip_abs4",
            title="Test",
            description="This is the 4 test strds",
            semantictype="sum",
        )
        cls.runModule(
            "t.create",
            verbose=True,
            overwrite=True,
            type="strds",
            temporaltype="absolute",
            output="precip_abs5",
            title="Test",
            description="This is the 5 test strds",
            semantictype="sum",
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule(
            "t.remove",
            flags="rf",
            verbose=True,
            type="strds",
            inputs="precip_abs2,precip_abs4,precip_abs5,precip_abs6",
        )
        # cls.runModule("t.unregister", type="raster", maps="prec_1,prec_2")
        cls.runModule("g.remove", flags="f", type="raster", name="prec_1,prec_2")

        cls.del_temp_region()

    def test_rename(self):
        """Test renaming a space time raster dataset"""
        rename_module = SimpleModule(
            "t.rename",
            overwrite=True,
            type="strds",
            input="precip_abs3",
            output="precip_abs6",
        )
        self.assertModule(rename_module)

        strds = tgis.open_old_stds("precip_abs6", type="strds")

        print(gs.read_command("t.info", type="strds", input="precip_abs6"))

        self.assertEqual(strds.metadata.get_description(), "This is the 3 test strds")
        self.assertTrue("t.rename" in strds.metadata.get_command())

    def test_rename_with_overwrite(self):
        """Test renaming a space time raster dataset while overwriting
        an existing one"""
        rename_module = SimpleModule(
            "t.rename",
            overwrite=True,
            type="strds",
            input="precip_abs1",
            output="precip_abs2",
        )
        self.assertModule(rename_module)

        strds = tgis.open_old_stds("precip_abs2", type="strds")

        print(gs.read_command("t.info", type="strds", input="precip_abs2"))

        self.assertEqual(strds.metadata.get_description(), "This is the 1 test strds")
        self.assertTrue("t.rename" in strds.metadata.get_command())

    def test_fail_overwrite_without_flag(self):
        """Test that renaming a space time raster dataset fails if the
        output stds exists and the overwrite flag is not set"""
        rename_module = SimpleModule(
            "t.rename",
            type="strds",
            input="precip_abs2",
            output="precip_abs4",
        )
        self.assertModuleFail(rename_module)

    def test_fail_rename_to_other_mapset(self):
        """Test that renaming a space time raster dataset fails if the
        output name contains the name of a mapset that is not the current
        one"""
        rename_module = SimpleModule(
            "t.rename",
            type="strds",
            input="precip_abs2",
            output="precip_abs7@PERMANENT",
        )
        self.assertModuleFail(rename_module)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
