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

        cls.runModule("r.mapcalc", expression="a_1 = 100", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_2 = 200", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_3 = 300", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_4 = 400", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_5 = null()", overwrite=True)

        cls.runModule("t.create", type="strds", temporaltype="absolute",  
                                 output="A", title="A test", description="A test",  
                                 overwrite=True)
        cls.runModule("t.register",  flags="i", type="raster", input="A",  
                                     maps="a_1,a_2,a_3,a_4,a_5", start="2001-01-01", 
                                     increment="3 months", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", type="strds",  
                                   inputs="A")
        cls.del_temp_region()
        
    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove", flags="rf", type="stvds",  
                                   inputs="result")

    def test_simple_points(self):
        self.assertModule("t.rast.to.vect", input="A", output="result", 
                          type="point", flags="n", column="values",
                          basename="test",
                          nprocs=1, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-04-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=5
                        points=384
                        primitives=384"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_simple_area(self):
        self.assertModule("t.rast.to.vect", input="A", output="result", 
                          type="area", flags="n", column="values",
                          basename="test",
                          nprocs=1, overwrite=True, verbose=True)
                          
        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")
        
        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-04-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=5
                        boundaries=4
                        centroids=4
                        primitives=8
                        nodes=4
                        areas=4
                        islands=4"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_simple_area_smooth(self):
        """Test areas with smooth corners"""
        self.assertModule("t.rast.to.vect", input="A", output="result", 
                          type="area", flags="s", column="values",
                          basename="test",
                          nprocs=1, overwrite=True, verbose=True)
                          
        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")
        
        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-01-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=4
                        boundaries=4
                        centroids=4
                        primitives=8
                        nodes=4
                        areas=4
                        islands=4"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_parallel(self):
        self.assertModule("t.rast.to.vect",  input="A", output="result", 
                          type="point", flags="t", column="values",
                          basename="test",
                          nprocs=4, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-01-01 00:00:00'
                        granularity='3 months'
                        number_of_maps=4
                        points=384
                        primitives=384"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_num_suffix(self):
        self.assertModule("t.rast.to.vect",  input="A", output="result", 
                          type="point", flags="t", column="values",
                          basename="test", suffix="num%03",
                          nprocs=4, overwrite=True, verbose=True)
        self.assertVectorExists("test_001")

    def test_time_suffix(self):
        self.assertModule("t.rast.to.vect",  input="A", output="result", 
                          type="point", flags="t", column="values",
                          basename="test", suffix="time",
                          nprocs=4, overwrite=True, verbose=True)
        self.assertVectorExists("test_2001_01_01T00_00_00")


class TestRasterToVectorFails(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

        cls.runModule("r.mapcalc", expression="a_1 = 100", overwrite=True)

        cls.runModule("t.create",  type="strds", temporaltype="absolute",  
                                 output="A", title="A test", description="A test",  
                                 overwrite=True)
        cls.runModule("t.register",  flags="i", type="raster", input="A",  
                                     maps="a_1", start="2001-01-01", 
                                     increment="3 months", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", type="strds",
                      inputs="A")
        cls.del_temp_region()

    def test_error_handling(self):
        # No basename, no type
        self.assertModuleFail("t.rast.to.vect", input="A", output="result")


    def test_empty_strds(self):
        """Test for empty strds"""
        self.assertModule("t.rast.to.vect", input="A", output="result", 
                          type="point", flags="n", column="values",
                          basename="test",
                          where="start_time > '2010-01-01'",
                          nprocs=1, overwrite=True, verbose=True)

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
