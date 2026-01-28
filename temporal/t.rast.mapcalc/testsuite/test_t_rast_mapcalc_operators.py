"""Test t.rast.mapcalc temporal operators.

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


class TestRasterMapcalcOperators(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        cls.use_temp_region()
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

        # Generate data
        cls.runModule(
            "r.mapcalc", expression="prec_1 = rand(0, 550)", flags="s", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", expression="prec_2 = rand(0, 450)", flags="s", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", expression="prec_3 = rand(0, 320)", flags="s", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", expression="prec_4 = rand(0, 510)", flags="s", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", expression="prec_5 = rand(0, 300)", flags="s", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", expression="prec_6 = rand(0, 650)", flags="s", overwrite=True
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
            inputs="precip_abs1,precip_abs2,precip_abs3",
            quiet=True,
        )
        cls.runModule(
            "g.remove", flags="f", type="raster", pattern="prec_*", quiet=True
        )
        cls.runModule(
            "g.remove", flags="f", type="raster", pattern="new_prec_*", quiet=True
        )
        cls.del_temp_region()

    def test_start_time_end_time_operators(self):
        """Test start_time() and end_time() temporal operators."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs3",
            expression="if(start_time() >= 0 && end_time() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
            basename="new_prec",
            method="equal",
            nprocs=5,
            verbose=True,
            overwrite=True,
        )
        # Verify output STRDS was created
        tinfo_string = """number_of_maps=6
temporal_type=absolute
name=precip_abs3"""
        info = SimpleModule("t.info", type="strds", input="precip_abs3", flags="g")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )

    def test_start_time_components(self):
        """Test start_year(), start_month(), start_day(), start_hour(), start_minute(), start_second() operators."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs3",
            expression="if(start_year()>=0&&start_month()>=0&&start_day()>=0&&start_hour()>=0&&start_minute()>=0&&start_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
            basename="new_prec",
            method="equal",
            nprocs=5,
            verbose=True,
            overwrite=True,
        )
        # Verify output STRDS was created
        tinfo_string = """number_of_maps=6
name=precip_abs3"""
        info = SimpleModule("t.info", type="strds", input="precip_abs3", flags="g")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )

    def test_end_time_components(self):
        """Test end_year(), end_month(), end_day(), end_hour(), end_minute(), end_second() operators."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs3",
            expression="if(end_year()>=0&&end_month()>=0&&end_day()>=0&&end_hour()>=0&&end_minute()>=0&&end_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
            basename="new_prec",
            method="equal",
            nprocs=5,
            verbose=True,
            overwrite=True,
        )
        # Verify output STRDS was created
        tinfo_string = """number_of_maps=6
name=precip_abs3"""
        info = SimpleModule("t.info", type="strds", input="precip_abs3", flags="g")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )

    def test_start_doy_dow_operators(self):
        """Test start_doy() and start_dow() temporal operators."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs3",
            expression="if(start_doy() >= 0 && start_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
            basename="new_prec",
            method="equal",
            nprocs=5,
            verbose=True,
            overwrite=True,
        )
        # Verify output STRDS was created
        tinfo_string = """number_of_maps=6
name=precip_abs3"""
        info = SimpleModule("t.info", type="strds", input="precip_abs3", flags="g")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )

    def test_end_doy_dow_operators(self):
        """Test end_doy() and end_dow() temporal operators."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs3",
            expression="if(end_doy() >= 0 && end_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
            basename="new_prec",
            method="equal",
            nprocs=5,
            verbose=True,
            overwrite=True,
        )
        # Verify output STRDS was created
        tinfo_string = """number_of_maps=6
name=precip_abs3"""
        info = SimpleModule("t.info", type="strds", input="precip_abs3", flags="g")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )

    def test_map_comparison_operator(self):
        """Test comparison operator with map names."""
        self.assertModule(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1",
            output="precip_abs3",
            expression="if(precip_abs1 == prec_1, prec_1, null())",
            basename="new_prec",
            verbose=True,
            overwrite=True,
        )
        # Verify output STRDS was created
        info = SimpleModule("t.info", type="strds", input="precip_abs3", flags="g")
        info.run()
        self.assertEqual(info.returncode, 0)
        strds_info = gs.parse_key_val(info.outputs.stdout, sep="=")
        self.assertEqual(strds_info["name"], "precip_abs3")
        # With -n flag, null maps are registered, so should have 6 maps
        self.assertEqual(int(strds_info["number_of_maps"]), 6)


if __name__ == "__main__":
    test()
