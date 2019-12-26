"""Test t.rast.aggregation

(C) 2014 by the GRASS Development Team
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

class TestAggregationAbsolute(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,
                      t=50,  res=10,  res3=10)
        cls.runModule("r.mapcalc", expression="a1 = 100.0",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a2 = 200.0",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a3 = 300.0",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a4 = 400.0",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a5 = 500.0",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a6 = 600.0",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a7 = null()",  overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="absolute",
                                    output="A",  title="A test",
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", flags="i",  type="raster",  input="A",
                                     maps="a1,a2,a3,a4,a5,a6,a7",
                                     start="2001-01-15 12:05:45",
                                     increment="14 days",
                                     overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", type="strds", inputs="A")

    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove", flags="rf", type="strds", inputs="B")

    def test_disaggregation(self):
        """Disaggregation with empty maps"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="2 days",
                          method="average",
                          sampling=["overlaps","overlapped","during"],
                          nprocs=2, flags="n")

        tinfo_string="""start_time='2001-01-15 00:00:00'
                        end_time='2001-04-25 00:00:00'
                        granularity='2 days'
                        aggregation_type=average
                        number_of_maps=50
                        map_time=interval
                        min_min=100.0
                        min_max=600.0
                        max_min=100.0
                        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_aggregation_1month(self):
        """Aggregation one month"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="1 months",
                          method="maximum", sampling=["contains"],
                          file_limit=0, nprocs=3)

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2001-04-01 00:00:00'
                        granularity='1 month'
                        map_time=interval
                        aggregation_type=maximum
                        number_of_maps=3
                        min_min=100.0
                        min_max=500.0
                        max_min=100.0
                        max_max=500.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

        # Check the map names are correct
        lister = SimpleModule("t.rast.list", input="B", columns="name",
                              flags="u")
        self.runModule(lister)
        #print lister.outputs.stdout
        maps="b_2001_01" + os.linesep + "b_2001_02" + os.linesep + \
             "b_2001_03" + os.linesep
        self.assertEqual(maps, lister.outputs.stdout)

    def test_aggregation_1month_time(self):
        """Aggregation one month time suffix"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="1 months",
                          method="maximum", sampling=["contains"],
                          file_limit=0, nprocs=3, suffix='time')
        self.assertRasterExists('b_2001_01_01T00_00_00')

    def test_aggregation_2months(self):
        """Aggregation two month"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="2 months",
                          method="minimum", sampling=["contains"],
                          nprocs=4, offset=10, suffix='num%02')

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2001-05-01 00:00:00'
                        granularity='2 months'
                        map_time=interval
                        aggregation_type=minimum
                        number_of_maps=2
                        min_min=100.0
                        min_max=500.0
                        max_min=100.0
                        max_max=500.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

        # Check the map names are correct
        lister = SimpleModule("t.rast.list", input="B", columns="name",
                              flags="u")
        self.runModule(lister)
        #print lister.outputs.stdout
        maps="b_11" + os.linesep + "b_12" + os.linesep
        self.assertEqual(maps, lister.outputs.stdout)

    def test_aggregation_3months(self):
        """Aggregation three month"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="3 months",
                          method="sum", sampling=["contains"],
                          file_limit=0, nprocs=9, offset=100,
                          suffix='num%03')

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2001-04-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        aggregation_type=sum
                        number_of_maps=1
                        min_min=1500.0
                        min_max=1500.0
                        max_min=1500.0
                        max_max=1500.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

        # Check the map names are correct
        lister = SimpleModule("t.rast.list", input="B", columns="name",
                              flags="u")
        self.runModule(lister)
        #print lister.outputs.stdout
        maps="b_101" + os.linesep
        self.assertEqual(maps, lister.outputs.stdout)

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
