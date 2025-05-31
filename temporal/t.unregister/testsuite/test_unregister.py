"""Test t.unregister

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import os

import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestUnregister(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()

    def setUp(self):
        """Create input data"""
        self.runModule("r.mapcalc", expression="a1 = 100", overwrite=True)
        self.runModule("r.mapcalc", expression="a2 = 200", overwrite=True)
        self.runModule("r.mapcalc", expression="a3 = 300", overwrite=True)
        self.runModule("r.mapcalc", expression="a4 = 400", overwrite=True)
        self.runModule("r.mapcalc", expression="a5 = 500", overwrite=True)
        self.runModule("r.mapcalc", expression="a6 = 600", overwrite=True)

        self.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="A",
            title="A test",
            description="A test",
            overwrite=True,
        )
        self.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="A",
            maps="a1,a2,a3,a4,a5,a6",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )

        self.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="B",
            title="B test",
            description="B test",
            overwrite=True,
        )
        self.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="B",
            maps="a1,a2,a3,a4,a5,a6",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )

    def tearDown(self):
        """Remove generated data"""
        self.runModule("g.remove", flags="f", type="raster", name="a1,a2,a3,a4,a5,a6")

    def test_1(self):
        """Perform several unregistration operations"""

        # Prepare some strings for the tests
        new_line = os.linesep
        a = ["a1", "a2", "a3", "a4", "a5", "a6"]
        al = new_line.join(a)
        al += new_line

        a123 = new_line.join(a[:3])
        a123 += new_line

        a456 = new_line.join(a[3:])
        a456 += new_line

        # Unregister maps a1, a2 and a3 from STRDS A
        self.assertModule("t.unregister", input="A", maps="a1,a2,a3")

        lister = SimpleModule("t.rast.list", input="A", columns="name", flags="u")
        self.runModule(lister)
        self.assertEqual(a456, lister.outputs.stdout)

        # Check if all maps are present in STRDS B
        lister = SimpleModule("t.rast.list", input="B", columns="name", flags="u")
        self.runModule(lister)
        self.assertEqual(al, lister.outputs.stdout)

        # Check if maps a1, a2 and a3 are still present in the temporal database
        lister = SimpleModule(
            "t.list",
            type="raster",
            columns="name",
            where="mapset = '%s' AND (name = 'a1' OR name = 'a2' OR name = 'a3')"
            % (tgis.get_current_mapset()),
        )
        self.runModule(lister)
        self.assertEqual(a123, lister.outputs.stdout)

        # Unregister maps a1, a2 and a3 from the temporal database
        self.assertModule("t.unregister", maps="a1,a2,a3")

        lister = SimpleModule(
            "t.list",
            type="raster",
            columns="name",
            where="mapset = '%s' AND (name = 'a1' OR name = 'a2' OR name = 'a3')"
            % (tgis.get_current_mapset()),
        )
        self.runModule(lister)
        self.assertEqual("", lister.outputs.stdout)

        # Check that masp a1, a2 and a3 are not present in STRDS B
        lister = SimpleModule("t.rast.list", input="B", columns="name", flags="u")
        self.runModule(lister)
        self.assertEqual(a456, lister.outputs.stdout)

        # Remove STRDS A and B and check if maps a4, a5 and a6 are still in the temporal database
        self.assertModule("t.remove", type="strds", inputs="A,B")

        lister = SimpleModule(
            "t.list",
            type="raster",
            columns="name",
            where="mapset = '%s' AND (name = 'a4' OR name = 'a5' OR name = 'a6')"
            % (tgis.get_current_mapset()),
        )
        self.runModule(lister)
        self.assertEqual(a456, lister.outputs.stdout)

        # Unregister maps a4, a5 and a6 from the temporal database
        self.assertModule("t.unregister", maps="a4,a5,a6")

        lister = SimpleModule(
            "t.list",
            type="raster",
            columns="name",
            where="mapset = '%s' AND (name = 'a4' OR name = 'a5' OR name = 'a6')"
            % (tgis.get_current_mapset()),
        )
        self.runModule(lister)
        self.assertEqual("", lister.outputs.stdout)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
