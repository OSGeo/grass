"""Test t.rast.extract

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import grass.pygrass.modules as pymod
import subprocess
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

class TestRasterExtraction(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.gisenv",  set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)
        cls.runModule("r.mapcalc", expression="prec_1 = 100",  overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_2 = 200",  overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_3 = 300",  overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_4 = 400",  overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_5 = 500",  overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_6 = 600",  overwrite=True)

        cls.runModule("t.create", type="strds", temporaltype="absolute",  
                      output="precip_abs1", title="A test",
                      description="A test", overwrite=True)
        cls.runModule("t.register", flags="i", type="raster", input="precip_abs1",  
                      maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6", 
                      start="2001-01-01", increment="3 months", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()

    def setUp(self):
        """Create input data for transient groundwater flow computation
        """
        # Use always the current mapset as temporal database
        self.runModule("r.mapcalc", expression="prec_1 = 100",  overwrite=True)
        self.runModule("r.mapcalc", expression="prec_2 = 200",  overwrite=True)
        self.runModule("r.mapcalc", expression="prec_3 = 300",  overwrite=True)
        self.runModule("r.mapcalc", expression="prec_4 = 400",  overwrite=True)
        self.runModule("r.mapcalc", expression="prec_5 = 500",  overwrite=True)
        self.runModule("r.mapcalc", expression="prec_6 = 600",  overwrite=True)

        self.runModule("t.create",  type="strds",  temporaltype="absolute",  
                                     output="precip_abs1",  title="A test",  description="A test",  overwrite=True)
        self.runModule("t.register",  flags="i",  type="raster",  input="precip_abs1",  
                                     maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",  
                                     start="2001-01-01", increment="3 months",  overwrite=True)

    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove",  flags="rf",  type="strds",  
                                   inputs="precip_abs2")

    def test_selection(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.rast.extract",  input="precip_abs1",  output="precip_abs2", 
                                      where="start_time > '2001-06-01'")

        #self.assertModule("t.info",  flags="g",  input="precip_abs2")

        tinfo_string="""start_time='2001-07-01 00:00:00'
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
        number_of_maps=4
        nsres_min=10.0
        nsres_max=10.0
        ewres_min=10.0
        ewres_max=10.0
        min_min=300.0
        min_max=600.0
        max_min=300.0
        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", input="precip_abs2")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_selection_and_expression(self):
        """Perform a selection by datetime and a r.mapcalc expression"""
        self.assertModule("t.rast.extract",  input="precip_abs1",  output="precip_abs2", 
                                      where="start_time > '2001-06-01'",  
                                      expression=" if(precip_abs1 > 400, precip_abs1, null())", 
                                      basename="new_prec",  nprocs=2,  overwrite=True)

        #self.assertModule("t.info",  flags="g",  input="precip_abs2")

        tinfo_string="""start_time='2002-01-01 00:00:00'
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
        number_of_maps=2
        nsres_min=10.0
        nsres_max=10.0
        ewres_min=10.0
        ewres_max=10.0
        min_min=500.0
        min_max=600.0
        max_min=500.0
        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", input="precip_abs2")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_expression_with_empty_maps(self):
        """Perform r.mapcalc expression and register empty maps"""
        self.assertModule("t.rast.extract",  flags="n",  input="precip_abs1",  output="precip_abs2",
                                      expression=" if(precip_abs1 > 400, precip_abs1, null())", 
                                      basename="new_prec",  nprocs=2,  overwrite=True)

        #self.assertModule("t.info",  flags="g",  input="precip_abs2")

        tinfo_string="""start_time='2001-01-01 00:00:00'
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
        min_min=500.0
        min_max=600.0
        max_min=500.0
        max_max=600.0"""

        info = SimpleModule("t.info", flags="g", input="precip_abs2")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        
    def test_time_suffix_with_expression(self):
        """Perform extract with time suffix support and test if maps exists"""
        self.assertModule("t.rast.extract",  flags="n",  input="precip_abs1",
                          output="precip_abs2", basename="new_prec",
                          nprocs=2,  overwrite=True, suffix="time",
                          expression="if(precip_abs1 > 400, precip_abs1, null())")
        self.assertRasterExists('new_prec_2001_01_01T00_00_00')
        self.assertRasterDoesNotExist('new_prec_2001_01')
        
    def test_num_suffix_with_expression(self):
        """Perform extract with time suffix support and test if maps exists"""
        self.assertModule("t.rast.extract",  flags="n",  input="precip_abs1",
                          output="precip_abs2", basename="new_prec",
                          nprocs=2,  overwrite=True, suffix='num%03',
                          expression="if(precip_abs1 > 400, precip_abs1, null())")
        self.assertRasterExists('new_prec_001')
        self.assertRasterDoesNotExist('new_prec_00001')



class TestRasterExtractionFails(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.gisenv",  set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()

    def test_error_handling(self):
        """Perform r.mapcalc expression and register empty maps"""
        # No input
        self.assertModuleFail("t.rast.extract",  output="precip_abs2", basename="new_prec")
        # No output
        self.assertModuleFail("t.rast.extract",  input="precip_abs1",  basename="new_prec")
        # No basename
        self.assertModuleFail("t.rast.extract",  input="precip_abs1",  output="precip_abs2", 
                          expression=" if(precip_abs1 > 400, precip_abs1, null())")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
