"""Test t.snap

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
                                     increment="14 days",  
                                     overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()        
        cls.runModule("t.remove", flags="rf", type="strds", inputs="A")
        
    def test_1_metadata(self):
        """Set title, description and aggregation"""
        
        A = tgis.open_old_stds("A", type="strds")
        A.select()
        self.assertEqual(A.get_map_time(), "point") 
        
        self.assertModule("t.snap", input="A", type="strds")
        
        A.select()
        self.assertEqual(A.get_map_time(), "interval") 


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
        
    def test_1_metadata(self):
        """Set title, description and aggregation"""
        
        A = tgis.open_old_stds("A", type="strds")
        A.select()
        self.assertEqual(A.get_map_time(), "point") 
        
        self.assertModule("t.snap", input="A", type="strds")
        
        A.select()
        self.assertEqual(A.get_map_time(), "interval") 
 

class TestSnapAbsoluteSTR3DS(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  
                      t=50,  res=10,  res3=10)
        cls.runModule("r3.mapcalc", expression="a1 = 100",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a2 = 200",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a3 = 300",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a4 = 400",  overwrite=True)

        cls.runModule("t.create",  type="str3ds",  temporaltype="absolute",  
                                    output="A",  title="A test",  
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", type="raster_3d",  input="A",  
                                     maps="a1,a2,a3,a4",
                                     start="2001-01-01", 
                                     increment="14 days",  
                                     overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()        
        cls.runModule("t.remove", flags="rf", type="str3ds", inputs="A")
        
    def test_1_metadata(self):
        """Set title, description and aggregation"""
        
        A = tgis.open_old_stds("A", type="str3ds")
        A.select()
        self.assertEqual(A.get_map_time(), "point") 
        
        self.assertModule("t.snap", input="A", type="str3ds")
        
        A.select()
        self.assertEqual(A.get_map_time(), "interval") 


class TestSnapRelativeSTR3DS(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  
                      t=50,  res=10,  res3=10)
        cls.runModule("r3.mapcalc", expression="a1 = 100",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a2 = 200",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a3 = 300",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a4 = 400",  overwrite=True)

        cls.runModule("t.create",  type="str3ds",  temporaltype="relative",  
                                    output="A",  title="A test",  
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", type="raster_3d",  input="A",  
                                     maps="a1,a2,a3,a4",
                                     start="0", 
                                     increment="14", unit="days",  
                                     overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()        
        cls.runModule("t.remove", flags="rf", type="str3ds", inputs="A")
        
    def test_1_metadata(self):
        """Set title, description and aggregation"""
        
        A = tgis.open_old_stds("A", type="str3ds")
        A.select()
        self.assertEqual(A.get_map_time(), "point") 
        
        self.assertModule("t.snap", input="A", type="str3ds")
        
        A.select()
        self.assertEqual(A.get_map_time(), "interval") 
 


class TestSnapAbsoluteSTVDS(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  
                      t=50,  res=10,  res3=10)
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a1')
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a2')
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a3')
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a4')

        cls.runModule("t.create",  type="stvds",  temporaltype="absolute",  
                                    output="A",  title="A test",  
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", type="vector",  input="A",  
                                     maps="a1,a2,a3,a4",
                                     start="2001-01-01", 
                                     increment="14 days",  
                                     overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()        
        cls.runModule("t.remove", flags="rf", type="stvds", inputs="A")
        
    def test_1_metadata(self):
        """Set title, description and aggregation"""
        
        A = tgis.open_old_stds("A", type="stvds")
        A.select()
        self.assertEqual(A.get_map_time(), "point") 
        
        self.assertModule("t.snap", input="A", type="stvds")
        
        A.select()
        self.assertEqual(A.get_map_time(), "interval") 


class TestSnapRelativeSTVDS(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  
                      t=50,  res=10,  res3=10)
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a1')
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a2')
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a3')
        cls.runModule("v.random", quiet=True, npoints=20, seed=1,  output='a4')

        cls.runModule("t.create",  type="stvds",  temporaltype="relative",  
                                    output="A",  title="A test",  
                                    description="A test",  overwrite=True)

        cls.runModule("t.register", type="vector",  input="A",  
                                     maps="a1,a2,a3,a4",
                                     start="0", 
                                     increment="14", unit="days",  
                                     overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()        
        cls.runModule("t.remove", flags="rf", type="stvds", inputs="A")
        
    def test_1_metadata(self):
        """Set title, description and aggregation"""
        
        A = tgis.open_old_stds("A", type="stvds")
        A.select()
        self.assertEqual(A.get_map_time(), "point") 
        
        self.assertModule("t.snap", input="A", type="stvds")
        
        A.select()
        self.assertEqual(A.get_map_time(), "interval") 
 
 
if __name__ == '__main__':
    from grass.gunittest.main import test
    test()

