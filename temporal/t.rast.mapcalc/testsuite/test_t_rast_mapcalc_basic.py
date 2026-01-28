"""Test t.rast.mapcalc basic functionality.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author GRASS Development Team
"""

import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.main import test


class TestRasterMapcalcBasic(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        cls.use_temp_region()
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

        # Generate data using rand() to match original shell test
        cls.runModule("r.mapcalc", expression="prec_1 = rand(0, 550)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_2 = rand(0, 450)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_3 = rand(0, 320)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_4 = rand(0, 510)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_5 = rand(0, 300)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_6 = rand(0, 650)", overwrite=True)

        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
            overwrite=True,
        )
        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs2",
            title="A test",
            description="A test",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            type="raster",
            input="precip_abs2",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule(
            "t.remove",
            flags="rf",
            type="strds",
            inputs="precip_abs1,precip_abs2,precip_abs3,precip_abs4",
            quiet=True,
        )
        cls.runModule(
            "g.remove", flags="f", type="raster", pattern="prec_*", quiet=True
        )
        cls.runModule(
            "g.remove", flags="f", type="raster", pattern="new_prec_*", quiet=True
        )
        cls.del_temp_region()

    def test_basic_addition(self):
        """Test basic addition of two STRDS with -n flag (register null maps)."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="n",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs3",
            expression=" precip_abs1 + precip_abs2",
            basename="new_prec",
            method="equal",
            nprocs=5,
            overwrite=True,
        )
        # Verify output STRDS was created and has expected number of maps
        tinfo_string = """number_of_maps=6
temporal_type=absolute
name=precip_abs3"""
        info = SimpleModule("t.info", type="strds", input="precip_abs3", flags="g")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )

    def test_division_with_three_inputs(self):
        """Test division expression with three input STRDS using -s flag."""
        # Create precip_abs3 first to make this test independent
        self.assertModule(
            "t.rast.mapcalc",
            flags="n",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs3",
            expression=" precip_abs1 + precip_abs2",
            basename="new_prec",
            method="equal",
            nprocs=5,
            overwrite=True,
        )
        self.assertModule(
            "t.rast.mapcalc",
            flags="s",
            inputs="precip_abs1,precip_abs2,precip_abs3",
            output="precip_abs4",
            expression=" (precip_abs1 + precip_abs2) / precip_abs2",
            basename="new_prec",
            method="equal",
            nprocs=5,
            overwrite=True,
        )
        # Verify output STRDS was created and has expected number of maps
        tinfo_string = """number_of_maps=6
temporal_type=absolute
name=precip_abs4"""
        info = SimpleModule("t.info", type="strds", input="precip_abs4", flags="g")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )

    def test_null_multiplication(self):
        """Test multiplication with null() function using -s flag."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="s",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs4",
            expression=" (precip_abs1 + precip_abs2) * null()",
            basename="new_prec",
            method="equal",
            nprocs=5,
            overwrite=True,
        )
        # Verify output STRDS was created
        info = SimpleModule("t.info", type="strds", input="precip_abs4", flags="g")
        info.run()
        self.assertEqual(info.returncode, 0)
        strds_info = gs.parse_key_val(info.outputs.stdout, sep="=")
        self.assertEqual(strds_info["name"], "precip_abs4")
        # With null() multiplication without -n flag, null maps are not registered
        self.assertEqual(int(strds_info["number_of_maps"]), 0)

    def test_null_multiplication_with_null_flag(self):
        """Test multiplication with null() function using -sn flags."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs4",
            expression=" (precip_abs1 + precip_abs2) * null()",
            basename="new_prec",
            method="equal",
            nprocs=5,
            overwrite=True,
        )
        # Verify output STRDS was created
        info = SimpleModule("t.info", type="strds", input="precip_abs4", flags="g")
        info.run()
        self.assertEqual(info.returncode, 0)
        strds_info = gs.parse_key_val(info.outputs.stdout, sep="=")
        self.assertEqual(strds_info["name"], "precip_abs4")
        # With -n flag, null maps are registered, so should have 6 maps
        self.assertEqual(int(strds_info["number_of_maps"]), 6)

    def test_failure_on_missing_map(self):
        """Test that the module fails when a required map is missing."""
        # Remove a raster map that is required
        self.runModule(
            "g.remove", flags="f", type="raster", name="prec_1", overwrite=True
        )
        # This should fail
        self.assertModuleFail(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs4",
            expression=" (precip_abs1 + precip_abs2) * null()",
            basename="new_prec",
            method="equal",
            nprocs=5,
            overwrite=True,
        )
        # Recreate the map so other tests aren't affected
        self.runModule("r.mapcalc", expression="prec_1 = rand(0, 550)", overwrite=True)


if __name__ == "__main__":
    test()
