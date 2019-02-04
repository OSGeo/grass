"""Test t.rast3d.extract

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import grass.pygrass.modules as pymod
import subprocess
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

class TestRaster3dExtraction(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()

    def setUp(self):
        """Create input data for transient groundwater flow computation
        """
        # Use always the current mapset as temporal database
        self.runModule("r3.mapcalc", expression="a1 = 100",  overwrite=True)
        self.runModule("r3.mapcalc", expression="a2 = 200",  overwrite=True)
        self.runModule("r3.mapcalc", expression="a3 = 300",  overwrite=True)
        self.runModule("r3.mapcalc", expression="a4 = 400",  overwrite=True)
        self.runModule("r3.mapcalc", expression="a5 = 500",  overwrite=True)
        self.runModule("r3.mapcalc", expression="a6 = 600",  overwrite=True)

        self.runModule("t.create",  type="str3ds",  temporaltype="absolute",  
                                     output="A",  title="A test",  description="A test",  overwrite=True)
        self.runModule("t.register",  flags="i",  type="raster_3d",  input="A",  
                                     maps="a1,a2,a3,a4,a5,a6",  
                                     start="2001-01-01", increment="3 months",  overwrite=True)

    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove",  flags="rf",  type="str3ds",  
                                   inputs="A,B")

    def test_selection(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.rast3d.extract",  input="A",  output="B", 
                                      where="start_time > '2001-06-01'")

        #self.assertModule("t.info",  flags="g",  input="B")

        tinfo_string="""start_time='2001-07-01 00:00:00'
        end_time='2002-07-01 00:00:00'
        granularity='3 months'
        map_time=interval
        aggregation_type=None
        number_of_maps=4
        min_min=300.0
        min_max=600.0
        max_min=300.0
        max_max=600.0"""

        info = SimpleModule("t.info", flags="g",  type="str3ds", input="B")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_selection_and_expression(self):
        """Perform a selection by datetime and a r3.mapcalc expression"""
        self.assertModule("t.rast3d.extract",  input="A",  output="B", 
                                      where="start_time > '2001-06-01'",  
                                      expression=" if(A > 400, A, null())", 
                                      basename="b",  nprocs=2,  overwrite=True)

        #self.assertModule("t.info",  flags="g",  input="B")

        tinfo_string="""start_time='2002-01-01 00:00:00'
        end_time='2002-07-01 00:00:00'
        granularity='3 months'
        map_time=interval
        aggregation_type=None
        number_of_maps=2
        min_min=500.0
        min_max=600.0
        max_min=500.0
        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", type="str3ds", input="B")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_expression_with_empty_maps(self):
        """Perform r3.mapcalc expression and register empty maps"""
        self.assertModule("t.rast3d.extract",  flags="n",  input="A",  output="B",
                                      expression=" if(A > 400, A, null())", 
                                      basename="b",  nprocs=2,  overwrite=True)

        #self.assertModule("t.info",  flags="g",  input="B")

        tinfo_string="""start_time='2001-01-01 00:00:00'
        end_time='2002-07-01 00:00:00'
        granularity='3 months'
        map_time=interval
        aggregation_type=None
        number_of_maps=6
        min_min=500.0
        min_max=600.0
        max_min=500.0
        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", type="str3ds", input="B")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_time_suffix_with_expression(self):
        """Perform extract with time suffix support and test if maps exists"""
        self.assertModule("t.rast3d.extract",  flags="n",  input="A", nprocs=2,
                          output="B", basename="b", overwrite=True,
                          suffix="time", expression="if(A > 400, A, null())")
        self.assertRaster3dExists('b_2001_01_01T00_00_00')
        self.assertRaster3dDoesNotExist('b_2001_01')

    def test_num_suffix_with_expression(self):
        """Perform extract with time suffix support and test if maps exists"""
        self.assertModule("t.rast3d.extract",  flags="n",  input="A", nprocs=2,
                          output="B", basename="b", overwrite=True,
                          suffix='num%03', expression="if(A > 400, A, null())")
        self.assertRaster3dExists('b_001')
        self.assertRaster3dDoesNotExist('b_00001')


class TestRaster3dExtractionFails(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()

    def test_error_handling(self):
        """Perform r3.mapcalc expression and register empty maps"""
        # No input
        self.assertModuleFail("t.rast3d.extract",  output="B", basename="b")
        # No output
        self.assertModuleFail("t.rast3d.extract",  input="A",  basename="b")
        # No basename
        self.assertModuleFail("t.rast3d.extract",  input="A",  output="B", 
                          expression=" if(A > 400, A, null())")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
