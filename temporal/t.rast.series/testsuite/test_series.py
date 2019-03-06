"""Test t.rast.series

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


class TestSnapAbsoluteSTRDS(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,
                      t=50,  res=10,  res3=10)
        cls.runModule("r.mapcalc", expression="a1 = 100",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a2 = 200",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a3 = 300",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a4 = 400",  overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="absolute",
                                    output="A",  title="A test",
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", type="raster",  input="A",
                                    maps="a1,a2,a3,a4",
                                    start="2001-01-01",
                                    increment="1 month",
                                    flags="i",
                                    overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", type="strds", inputs="A")
        cls.runModule("t.unregister", type="raster",
                      maps="series_average,series_maximum,series_minimum,series_minimum_2")
        cls.runModule("g.remove", flags="f", type="raster", name="series_average")
        cls.runModule("g.remove", flags="f", type="raster", name="series_maximum")
        cls.runModule("g.remove", flags="f", type="raster", name="series_minimum")
        cls.runModule("g.remove", flags="f", type="raster", name="series_minimum_2")
        cls.runModule("g.remove", flags="f", type="raster", name="series_quantile")

    def test_time_stamp(self):
        self.assertModule("t.rast.series", input="A", method="average",
                          output="series_time_stamp", where="start_time > '2001-02-01'")

        tinfo_string="""start_time='2001-02-01 00:00:00'
                        end_time='2001-05-01 00:00:00'"""

        info = SimpleModule("t.info", flags="g", type="raster", input="series_time_stamp")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_average(self):
        self.assertModule("t.rast.series", input="A", method="average",
                          output="series_average")

        self.assertRasterMinMax(map="series_average", refmin=250, refmax=250,
                                msg="Average must be 250")

    def test_maximum(self):
        self.assertModule("t.rast.series", input="A", method="maximum",
                          output="series_maximum")

        self.assertRasterMinMax(map="series_maximum", refmin=400, refmax=400,
                                msg="Maximum must be 400")

    def test_minimum(self):
        self.assertModule("t.rast.series", input="A", method="minimum",
                          output="series_minimum")

        self.assertRasterMinMax(map="series_minimum", refmin=100, refmax=100,
                                msg="Minimum must be 100")

    def test_multi_stats(self):
        self.assertModule("t.rast.series", input="A",
                          method=["average", "maximum", "minimum"],
                          output=["series_average", "series_maximum",
                                  "series_minimum"],
                          overwrite=True)

        self.assertRasterMinMax(map="series_average", refmin=250, refmax=250,
                                msg="Average must be 250")

        self.assertRasterMinMax(map="series_maximum", refmin=400, refmax=400,
                                msg="Maximum must be 400")

        self.assertRasterMinMax(map="series_minimum", refmin=100, refmax=100,
                                msg="Minimum must be 100")

    def test_minimum_where(self):
        self.assertModule("t.rast.series", input="A", method="minimum",
                          output="series_minimum_2",
                          where="start_time >= '2001-03-01'")

        self.assertRasterMinMax(map="series_minimum_2", refmin=300, refmax=300,
                                msg="Minimum must be 300")

    def test_quantile(self):
        self.assertModule("t.rast.series", input="A", method="quantile", quantile=0.5,
                          output="series_quantile")

        self.assertRasterMinMax(map="series_quantile", refmin=300, refmax=300,
                                msg="Minimum must be 300")


class TestSnapRelativeSTRDS(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,
                      t=50,  res=10,  res3=10)
        cls.runModule("r.mapcalc", expression="a1 = 100",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a2 = 200",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a3 = 300",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a4 = 400",  overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="relative",
                                    output="A",  title="A test",
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", type="raster",  input="A",
                                     maps="a1,a2,a3,a4",
                                     start="0",
                                     increment="14", unit="days",
                                     overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", type="strds", inputs="A")
        cls.runModule("t.unregister", type="raster",
                      maps="series_average,series_maximum,series_minimum,series_minimum_2")
        cls.runModule("g.remove", flags="f", type="raster", name="series_average")
        cls.runModule("g.remove", flags="f", type="raster", name="series_maximum")
        cls.runModule("g.remove", flags="f", type="raster", name="series_minimum")
        cls.runModule("g.remove", flags="f", type="raster", name="series_minimum_2")

    def test_average(self):
        self.assertModule("t.rast.series", input="A", method="average",
                          output="series_average")

        self.assertRasterMinMax(map="series_average", refmin=250, refmax=250,
                                msg="Average must be 250")

    def test_maximum(self):
        self.assertModule("t.rast.series", input="A", method="maximum",
                          output="series_maximum")

        self.assertRasterMinMax(map="series_maximum", refmin=400, refmax=400,
                                msg="Maximum must be 400")

    def test_minimum(self):
        self.assertModule("t.rast.series", input="A", method="minimum",
                          output="series_minimum")

        self.assertRasterMinMax(map="series_minimum", refmin=100, refmax=100,
                                msg="Minimum must be 100")

    def test_minimum_where(self):
        self.assertModule("t.rast.series", input="A", method="minimum",
                          output="series_minimum_2",
                          where="start_time >= 28")

        self.assertRasterMinMax(map="series_minimum_2", refmin=300, refmax=300,
                                msg="Minimum must be 300")



if __name__ == '__main__':
    from grass.gunittest.main import test
    test()

