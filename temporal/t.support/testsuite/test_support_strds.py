"""Test t.support

(C) 2015 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
import os
import grass.pygrass.modules as pymod
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestSupportAbsoluteSTRDS(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        os.putenv("GRASS_OVERWRITE", "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        cls.runModule("r.mapcalc", expression="a1 = 100", overwrite=True)
        cls.runModule("r.mapcalc", expression="a2 = 200", overwrite=True)
        cls.runModule("r.mapcalc", expression="a3 = 300", overwrite=True)
        cls.runModule("r.mapcalc", expression="a4 = 400", overwrite=True)

        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="A",
            title="A test",
            description="A test",
            overwrite=True,
        )

        cls.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="A",
            maps="a1,a2,a3,a4",
            start="2001-01-15 12:05:45",
            increment="14 days",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()
        cls.runModule("t.remove", flags="df", type="strds", inputs="A")

    def test_1_metadata(self):
        """Set title, description and aggregation"""

        title = "A new title"
        descr = "A new description"
        aggr_type = "average"
        semantic = "sum"

        self.assertModule(
            "t.support",
            input="A",
            title=title,
            description=descr,
            semantictype=semantic,
            aggr_type=aggr_type,
        )

        A = tgis.open_old_stds("A", type="strds")
        A.select()
        self.assertEqual(A.metadata.get_title(), title)
        self.assertEqual(A.metadata.get_description(), descr)
        self.assertEqual(A.metadata.get_aggregation_type(), aggr_type)
        self.assertEqual(A.base.get_semantic_type(), semantic)

    def test_2_update(self):
        """Set title, description and aggregation"""

        self.runModule("r.mapcalc", expression="a1 = 10", overwrite=True)
        self.runModule("r.mapcalc", expression="a2 = 20", overwrite=True)
        self.runModule("r.mapcalc", expression="a3 = 30", overwrite=True)
        self.runModule("r.mapcalc", expression="a4 = 40", overwrite=True)

        self.assertModule("t.support", input="A", flags="m")

        A = tgis.open_old_stds("A", type="strds")
        A.select()
        self.assertEqual(A.metadata.get_min_min(), 10)
        self.assertEqual(A.metadata.get_min_max(), 40)
        self.assertEqual(A.metadata.get_max_min(), 10)
        self.assertEqual(A.metadata.get_max_max(), 40)
        self.assertEqual(A.metadata.get_number_of_maps(), 4)

    def test_3_update(self):
        """Set title, description and aggregation"""

        self.runModule("g.remove", type="raster", name="a4", flags="f")

        self.assertModule("t.support", input="A", flags="m")

        A = tgis.open_old_stds("A", type="strds")
        A.select()
        self.assertEqual(A.metadata.get_min_min(), 10)
        self.assertEqual(A.metadata.get_min_max(), 30)
        self.assertEqual(A.metadata.get_max_min(), 10)
        self.assertEqual(A.metadata.get_max_max(), 30)
        self.assertEqual(A.metadata.get_number_of_maps(), 3)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
