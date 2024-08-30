"""Test t.copy

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Markus Metz
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestSTDSCopy(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        cls.use_temp_region()
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", pattern="prec_*")

    def setUp(self):
        """Create input data for t.copy"""
        # Use always the current mapset as temporal database
        self.runModule("r.mapcalc", expression="prec_1 = 100", overwrite=True)
        self.runModule("r.mapcalc", expression="prec_2 = 200", overwrite=True)
        self.runModule("r.mapcalc", expression="prec_3 = 300", overwrite=True)
        self.runModule("r.mapcalc", expression="prec_4 = 400", overwrite=True)
        self.runModule("r.mapcalc", expression="prec_5 = 500", overwrite=True)
        self.runModule("r.mapcalc", expression="prec_6 = 600", overwrite=True)

        self.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
            overwrite=True,
        )
        self.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )

    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove", flags="df", type="strds", inputs="precip_abs1")

    def test_copy(self):
        """Copy a strds"""
        self.assertModule(
            "t.copy",
            input="precip_abs1",
            output="precip_abs2",
            type="strds",
        )

        # self.assertModule("t.info",  flags="g",  input="precip_abs2")

        tinfo_string = """start_time='2001-01-01 00:00:00'
        end_time='2002-07-01 00:00:00'
        granularity='3 months'
        map_time=interval
        north=80.0
        south=0.0
        east=120.0
        west=0.0
        top=0.0
        bottom=0.0
        aggregation_type=None
        number_of_maps=6
        nsres_min=10.0
        nsres_max=10.0
        ewres_min=10.0
        ewres_max=10.0
        min_min=100.0
        min_max=600.0
        max_min=100.0
        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", input="precip_abs2")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="="
        )
        self.runModule("t.remove", flags="df", type="strds", inputs="precip_abs2")


class TestSTDSCopyFails(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        cls.use_temp_region()
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        cls.runModule("r.mapcalc", expression="prec_1 = 100", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_2 = 200", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_3 = 300", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_4 = 400", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_5 = 500", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_6 = 600", overwrite=True)

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
            "t.register",
            flags="i",
            type="raster",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()
        cls.runModule("t.remove", flags="df", type="strds", inputs="precip_abs1")
        cls.runModule("g.remove", flags="f", type="raster", pattern="prec_*")

    def test_error_handling(self):
        """Test arguments for t.copy"""
        # No input
        self.assertModuleFail("t.copy", output="precip_abs2")
        # No output
        self.assertModuleFail("t.copy", input="precip_abs1")
        # Wrong type
        self.assertModuleFail(
            "t.copy",
            input="precip_abs1",
            output="precip_abs2",
            type="stvds",
        )
        # Input does not exist
        self.assertModuleFail(
            "t.copy",
            input="precip_abs1@this_mapset_does_not_exist",
            output="precip_abs2",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
