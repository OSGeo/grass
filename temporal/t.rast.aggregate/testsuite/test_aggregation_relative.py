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

class TestAggregationRelative(TestCase):

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
        cls.runModule("r.mapcalc", expression="a5 = 500",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a6 = 600",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a7 = null()",  overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="relative",
                                    output="A",  title="A test",
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", flags="i",  type="raster",  input="A",
                                     maps="a1,a2,a3,a4,a5,a6,a7",
                                     start=0, unit="days", increment=3,
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

    def test_1(self):
        """Simple test"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity=6,
                          method="average", file_limit=0,
                          sampling=["overlaps","overlapped","contains"],
                          nprocs=2, verbose=True)

        tinfo_string="""start_time='0'
                        end_time='18'
                        unit=days
                        granularity=6
                        map_time=interval
                        aggregation_type=average
                        number_of_maps=3
                        min_min=150.0
                        min_max=550.0
                        max_min=150.0
                        max_max=550.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_2(self):
        """Simple test register null maps"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity=9,
                          method="maximum",
                          sampling=["contains"],
                          nprocs=4, flags="n", verbose=True)

        tinfo_string="""semantic_type=mean
                        start_time='0'
                        end_time='27'
                        unit=days
                        granularity=9
                        map_time=interval
                        aggregation_type=maximum
                        number_of_maps=3
                        min_min=300.0
                        min_max=600.0
                        max_min=300.0
                        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_3(self):
        """Simple test"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity=9,
                          method="maximum",
                          sampling=["contains"],
                          nprocs=4, verbose=True)

        tinfo_string="""semantic_type=mean
                        start_time='0'
                        end_time='18'
                        unit=days
                        granularity=9
                        map_time=interval
                        aggregation_type=maximum
                        number_of_maps=2
                        min_min=300.0
                        min_max=600.0
                        max_min=300.0
                        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_4(self):
        """Simple test"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity=21,
                          method="average",
                          sampling=["contains"],
                          nprocs=4, verbose=True)

        tinfo_string="""semantic_type=mean
                        start_time='0'
                        end_time='21'
                        unit=days
                        granularity=21
                        map_time=interval
                        aggregation_type=average
                        number_of_maps=2
                        min_min=350.0
                        min_max=350.0
                        max_min=350.0
                        max_max=350.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
