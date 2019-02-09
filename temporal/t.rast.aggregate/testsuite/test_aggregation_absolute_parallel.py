"""Test t.rast.aggregation

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from __future__ import print_function

import os
import grass.pygrass.modules as pymod
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from datetime import datetime

class TestAggregationAbsoluteParallel(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,
                      t=50,  res=10,  res3=10)

        name_list =  []
        for i in range(540):
            cls.runModule("r.mapcalc", expression="a%i = %i"%(i + 1, i + 1),  overwrite=True)
            name_list.append("a%i"%(i + 1))

        cls.runModule("t.create",  type="strds",  temporaltype="absolute",
                                    output="A",  title="A test",
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", flags="i",  type="raster",  input="A",
                                     maps=name_list,
                                     start="2001-01-01",
                                     increment="4 hours",
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

    def test_aggregation_12hours(self):
        """Aggregation one month"""
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="12 hours",
                          method="sum", sampling=["contains"],
                          nprocs=9, file_limit=2)

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2001-04-01 00:00:00'
                        granularity='12 hours'
                        map_time=interval
                        aggregation_type=sum
                        number_of_maps=180
                        min_min=6.0
                        min_max=1617.0
                        max_min=6.0
                        max_max=1617.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_aggregation_1day_4procs(self):
        """Aggregation one month"""
        start = datetime.now()
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="1 day",
                          method="sum", sampling=["contains"],
                          nprocs=4)
        end = datetime.now()

        delta = end - start
        print("test_aggregation_1day_4procs:",  delta.total_seconds())

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2001-04-01 00:00:00'
                        granularity='1 day'
                        map_time=interval
                        aggregation_type=sum
                        number_of_maps=90"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_aggregation_1day_3procs(self):
        """Aggregation one month"""
        start = datetime.now()
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="1 day",
                          method="sum", sampling=["contains"],
                          nprocs=3)
        end = datetime.now()

        delta = end - start
        print("test_aggregation_1day_3procs:",  delta.total_seconds())


        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2001-04-01 00:00:00'
                        granularity='1 day'
                        map_time=interval
                        aggregation_type=sum
                        number_of_maps=90
                        min_min=21.0
                        min_max=3225.0
                        max_min=21.0
                        max_max=3225.0"""

        info = SimpleModule("t.info", input="B", flags="g")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_aggregation_1day_2procs(self):
        """Aggregation one month"""
        start = datetime.now()
        self.assertModule("t.rast.aggregate", input="A", output="B",
                          basename="b", granularity="1 day",
                          method="sum", sampling=["contains"],
                          nprocs=2)
        end = datetime.now()

        delta = end - start
        print("test_aggregation_1day_2procs:",  delta.total_seconds())


        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2001-04-01 00:00:00'
                        granularity='1 day'
                        map_time=interval
                        aggregation_type=sum
                        number_of_maps=90
                        min_min=21.0
                        min_max=3225.0
                        max_min=21.0
                        max_max=3225.0"""

        info = SimpleModule("t.info", flags="g", input="B")
        #info.run()
        #print info.outputs.stdout
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")
if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
