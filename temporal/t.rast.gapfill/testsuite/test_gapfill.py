"""Test t.rast.to.vect

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import subprocess
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

class TestRasterToVector(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

    def setUp(self):
        self.runModule("r.mapcalc", expression="a_1 = 100", overwrite=True)
        self.runModule("r.mapcalc", expression="a_2 = 400", overwrite=True)
        self.runModule("r.mapcalc", expression="a_3 = 1200", overwrite=True)

        self.runModule("t.create", type="strds", temporaltype="absolute",  
                                 output="A", title="A test", description="A test",  
                                 overwrite=True)
        self.runModule("t.register",  flags="i", type="raster", input="A",  
                                     maps="a_1", start="2001-01-01", 
                                     increment="1 month", overwrite=True)
        self.runModule("t.register",  flags="i", type="raster", input="A",  
                                     maps="a_2", start="2001-04-01", 
                                     increment="1 months", overwrite=True)
        self.runModule("t.register",  flags="i", type="raster", input="A",  
                                     maps="a_3", start="2001-12-01", 
                                     increment="1 months", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        
    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove", flags="rf", type="strds",  
                                   inputs="A")

    def test_simple_2procs(self):
        self.assertModule("t.rast.gapfill", input="A", suffix="num%01",
                          basename="test", nprocs=2, verbose=True)

        #self.assertModule("t.info",  type="strds", flags="g",  input="A")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                                end_time='2002-01-01 00:00:00'
                                granularity='1 month'
                                map_time=interval
                                number_of_maps=12
                                min_min=100.0
                                min_max=1200.0
                                max_min=100.0
                                max_max=1200.0"""

        info = SimpleModule("t.info", flags="g", type="strds", input="A")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        text="""name|start_time|end_time|min|max
a_1|2001-01-01 00:00:00|2001-02-01 00:00:00|100.0|100.0
test_6_1|2001-02-01 00:00:00|2001-03-01 00:00:00|200.0|200.0
test_6_2|2001-03-01 00:00:00|2001-04-01 00:00:00|300.0|300.0
a_2|2001-04-01 00:00:00|2001-05-01 00:00:00|400.0|400.0
test_7_1|2001-05-01 00:00:00|2001-06-01 00:00:00|500.0|500.0
test_7_2|2001-06-01 00:00:00|2001-07-01 00:00:00|600.0|600.0
test_7_3|2001-07-01 00:00:00|2001-08-01 00:00:00|700.0|700.0
test_7_4|2001-08-01 00:00:00|2001-09-01 00:00:00|800.0|800.0
test_7_5|2001-09-01 00:00:00|2001-10-01 00:00:00|900.0|900.0
test_7_6|2001-10-01 00:00:00|2001-11-01 00:00:00|1000.0|1000.0
test_7_7|2001-11-01 00:00:00|2001-12-01 00:00:00|1100.0|1100.0
a_3|2001-12-01 00:00:00|2002-01-01 00:00:00|1200.0|1200.0

"""
        rast_list = SimpleModule("t.rast.list", columns=("name","start_time","end_time","min,max"),  input="A")
        self.assertModule(rast_list)
        self.assertLooksLike(text,  rast_list.outputs.stdout)

    def test_simple_where(self):
        self.assertModule("t.rast.gapfill", input="A",  where="start_time >= '2001-03-01'", 
                          basename="test", nprocs=1, verbose=True, suffix="num%01")

        #self.assertModule("t.info",  type="strds", flags="g",  input="A")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                                end_time='2002-01-01 00:00:00'
                                granularity='1 month'
                                map_time=interval
                                number_of_maps=10
                                min_min=100.0
                                min_max=1200.0
                                max_min=100.0
                                max_max=1200.0"""

        info = SimpleModule("t.info", flags="g", type="strds", input="A")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        text="""name|start_time|end_time|min|max
a_1|2001-01-01 00:00:00|2001-02-01 00:00:00|100.0|100.0
a_2|2001-04-01 00:00:00|2001-05-01 00:00:00|400.0|400.0
test_4_1|2001-05-01 00:00:00|2001-06-01 00:00:00|500.0|500.0
test_4_2|2001-06-01 00:00:00|2001-07-01 00:00:00|600.0|600.0
test_4_3|2001-07-01 00:00:00|2001-08-01 00:00:00|700.0|700.0
test_4_4|2001-08-01 00:00:00|2001-09-01 00:00:00|800.0|800.0
test_4_5|2001-09-01 00:00:00|2001-10-01 00:00:00|900.0|900.0
test_4_6|2001-10-01 00:00:00|2001-11-01 00:00:00|1000.0|1000.0
test_4_7|2001-11-01 00:00:00|2001-12-01 00:00:00|1100.0|1100.0
a_3|2001-12-01 00:00:00|2002-01-01 00:00:00|1200.0|1200.0
"""
        rast_list = SimpleModule("t.rast.list", columns=("name","start_time","end_time","min,max"),  input="A")
        self.assertModule(rast_list)
        self.assertLooksLike(text,  rast_list.outputs.stdout)

    def test_simple_where_2(self):
        self.assertModule("t.rast.gapfill", input="A",  where="start_time <= '2001-05-01'", 
                          basename="test", nprocs=1, verbose=True, suffix="num%01")

        #self.assertModule("t.info",  type="strds", flags="g",  input="A")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                                end_time='2002-01-01 00:00:00'
                                granularity='1 month'
                                map_time=interval
                                number_of_maps=5
                                min_min=100.0
                                min_max=1200.0
                                max_min=100.0
                                max_max=1200.0"""

        info = SimpleModule("t.info", flags="g", type="strds", input="A")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        text="""name|start_time|end_time|min|max
a_1|2001-01-01 00:00:00|2001-02-01 00:00:00|100.0|100.0
test_4_1|2001-02-01 00:00:00|2001-03-01 00:00:00|200.0|200.0
test_4_2|2001-03-01 00:00:00|2001-04-01 00:00:00|300.0|300.0
a_2|2001-04-01 00:00:00|2001-05-01 00:00:00|400.0|400.0
a_3|2001-12-01 00:00:00|2002-01-01 00:00:00|1200.0|1200.0
"""
        rast_list = SimpleModule("t.rast.list", columns=("name","start_time","end_time","min,max"),  input="A")
        self.assertModule(rast_list)
        self.assertLooksLike(text,  rast_list.outputs.stdout)

    def test_simple_empty(self):
        self.assertModule("t.rast.gapfill", input="A",  where="start_time >= '2001-10-01'", 
                          basename="test", nprocs=1, verbose=True, suffix="num%01")

        #self.assertModule("t.info",  type="strds", flags="g",  input="A")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                                end_time='2002-01-01 00:00:00'
                                granularity='1 month'
                                map_time=interval
                                number_of_maps=3
                                min_min=100.0
                                min_max=1200.0
                                max_min=100.0
                                max_max=1200.0"""

        info = SimpleModule("t.info", flags="g", type="strds", input="A")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        text="""name|start_time|end_time|min|max
a_1|2001-01-01 00:00:00|2001-02-01 00:00:00|100.0|100.0
a_2|2001-04-01 00:00:00|2001-05-01 00:00:00|400.0|400.0
a_3|2001-12-01 00:00:00|2002-01-01 00:00:00|1200.0|1200.0
"""
        rast_list = SimpleModule("t.rast.list", columns=("name","start_time","end_time","min,max"),  input="A")
        self.assertModule(rast_list)
        self.assertLooksLike(text,  rast_list.outputs.stdout)

    def test_simple_gran(self):
        self.assertModule("t.rast.gapfill", input="A",
                          basename="test", nprocs=2, verbose=True)

        #self.assertModule("t.info",  type="strds", flags="g",  input="A")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                                end_time='2002-01-01 00:00:00'
                                granularity='1 month'
                                map_time=interval
                                number_of_maps=12
                                min_min=100.0
                                min_max=1200.0
                                max_min=100.0
                                max_max=1200.0"""

        info = SimpleModule("t.info", flags="g", type="strds", input="A")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        text="""name|start_time|end_time|min|max
a_1|2001-01-01 00:00:00|2001-02-01 00:00:00|100.0|100.0
test_2001_02|2001-02-01 00:00:00|2001-03-01 00:00:00|200.0|200.0
test_2001_03|2001-03-01 00:00:00|2001-04-01 00:00:00|300.0|300.0
a_2|2001-04-01 00:00:00|2001-05-01 00:00:00|400.0|400.0
test_2001_05|2001-05-01 00:00:00|2001-06-01 00:00:00|500.0|500.0
test_2001_06|2001-06-01 00:00:00|2001-07-01 00:00:00|600.0|600.0
test_2001_07|2001-07-01 00:00:00|2001-08-01 00:00:00|700.0|700.0
test_2001_08|2001-08-01 00:00:00|2001-09-01 00:00:00|800.0|800.0
test_2001_09|2001-09-01 00:00:00|2001-10-01 00:00:00|900.0|900.0
test_2001_10|2001-10-01 00:00:00|2001-11-01 00:00:00|1000.0|1000.0
test_2001_11|2001-11-01 00:00:00|2001-12-01 00:00:00|1100.0|1100.0
a_3|2001-12-01 00:00:00|2002-01-01 00:00:00|1200.0|1200.0

"""
        rast_list = SimpleModule("t.rast.list", columns=("name","start_time","end_time","min,max"),  input="A")
        self.assertModule(rast_list)
        self.assertLooksLike(text,  rast_list.outputs.stdout)

    def test_simple_gran(self):
        self.assertModule("t.rast.gapfill", input="A", suffix="time",
                          basename="test", nprocs=2, verbose=True)

        #self.assertModule("t.info",  type="strds", flags="g",  input="A")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                                end_time='2002-01-01 00:00:00'
                                granularity='1 month'
                                map_time=interval
                                number_of_maps=12
                                min_min=100.0
                                min_max=1200.0
                                max_min=100.0
                                max_max=1200.0"""

        info = SimpleModule("t.info", flags="g", type="strds", input="A")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        text="""name|start_time|end_time|min|max
a_1|2001-01-01 00:00:00|2001-02-01 00:00:00|100.0|100.0
test_2001_02_01T00_00_00|2001-02-01 00:00:00|2001-03-01 00:00:00|200.0|200.0
test_2001_03_01T00_00_00|2001-03-01 00:00:00|2001-04-01 00:00:00|300.0|300.0
a_2|2001-04-01 00:00:00|2001-05-01 00:00:00|400.0|400.0
test_2001_05_01T00_00_00|2001-05-01 00:00:00|2001-06-01 00:00:00|500.0|500.0
test_2001_06_01T00_00_00|2001-06-01 00:00:00|2001-07-01 00:00:00|600.0|600.0
test_2001_07_01T00_00_00|2001-07-01 00:00:00|2001-08-01 00:00:00|700.0|700.0
test_2001_08_01T00_00_00|2001-08-01 00:00:00|2001-09-01 00:00:00|800.0|800.0
test_2001_09_01T00_00_00|2001-09-01 00:00:00|2001-10-01 00:00:00|900.0|900.0
test_2001_10_01T00_00_00|2001-10-01 00:00:00|2001-11-01 00:00:00|1000.0|1000.0
test_2001_11_01T00_00_00|2001-11-01 00:00:00|2001-12-01 00:00:00|1100.0|1100.0
a_3|2001-12-01 00:00:00|2002-01-01 00:00:00|1200.0|1200.0

"""
        rast_list = SimpleModule("t.rast.list", columns=("name","start_time","end_time","min,max"),  input="A")
        self.assertModule(rast_list)
        self.assertLooksLike(text,  rast_list.outputs.stdout)

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
