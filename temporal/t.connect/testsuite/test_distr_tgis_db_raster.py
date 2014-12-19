"""test distributed temporal databases with strds

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
import os
import grass.temporal as tgis

mapset_count = 0
class TestRasterExtraction(TestCase):

    @classmethod
    def setUpClass(cls):
        os.putenv("GRASS_OVERWRITE", "1")
        for i in range(1, 5): 
            cls.runModule("g.mapset", flags="c", mapset="test%i"%i)
            cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)
            cls.runModule("t.info", flags="s")
            cls.runModule("r.mapcalc", expression="a1 = 100")
            cls.runModule("r.mapcalc", expression="a2 = 200")
            cls.runModule("r.mapcalc", expression="a3 = 300")
            
            cls.runModule("t.create",  type="strds",  temporaltype="absolute",  
                                         output="A",  title="A test",  description="A test")
            cls.runModule("t.register",  flags="i",  type="raster",  input="A",  
                                         maps="a1,a2,a3",  
                                         start="2001-01-01", increment="%i months"%i)

        # Here we reuse two mapset to share a temporal databse between mapsets
        tgis.init()
        ciface = tgis.get_tgis_c_library_interface()
        cls.runModule("g.mapset", flags="c", mapset="test5")
        driver = ciface.get_driver_name("test1")
        database = ciface.get_database_name("test1")
        cls.runModule("t.connect",  driver=driver,  database=database)
        
        cls.runModule("g.mapset", flags="c", mapset="test6")
        driver = ciface.get_driver_name("test2")
        database = ciface.get_database_name("test2")
        cls.runModule("t.connect",  driver=driver,  database=database)

        for i in range(5, 7): 
            cls.runModule("g.mapset", mapset="test%i"%i)
            cls.runModule("r.mapcalc", expression="a1 = 100")
            cls.runModule("r.mapcalc", expression="a2 = 200")
            cls.runModule("r.mapcalc", expression="a3 = 300")
            
            cls.runModule("t.create",  type="strds",  temporaltype="absolute",  
                                         output="A",  title="A test",  description="A test")
            cls.runModule("t.register",  flags="i",  type="raster",  input="A",  
                                         maps="a1,a2,a3",  
                                         start="2001-01-01", increment="%i months"%i)

    def test_tlist(self):      
        self.runModule("g.mapset", mapset="test1")
        
        list_string = """A|test1|2001-01-01 00:00:00|2001-04-01 00:00:00|3
                                A|test2|2001-01-01 00:00:00|2001-07-01 00:00:00|3
                                A|test3|2001-01-01 00:00:00|2001-10-01 00:00:00|3
                                A|test4|2001-01-01 00:00:00|2002-01-01 00:00:00|3
                                A|test5|2001-01-01 00:00:00|2002-04-01 00:00:00|3
                                A|test6|2001-01-01 00:00:00|2002-07-01 00:00:00|3"""
                                
        entries = list_string.split("\n")
        
        t_list = SimpleModule("t.list", quiet=True, 
                                            columns=["name","mapset,start_time","end_time","number_of_maps"],  
                                            type="strds",  where='name = "A"')
        self.assertModule(t_list)
        
        out = t_list.outputs["stdout"].value
        
        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())
        
    def test_trast_list(self):      
        self.runModule("g.mapset", mapset="test1")

        list_string = """a1|test1|2001-01-01 00:00:00|2001-02-01 00:00:00
                                a2|test1|2001-02-01 00:00:00|2001-03-01 00:00:00
                                a3|test1|2001-03-01 00:00:00|2001-04-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.rast.list", quiet=True, flags="s",  input="A@test1")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test2|2001-01-01 00:00:00|2001-03-01 00:00:00
                                a2|test2|2001-03-01 00:00:00|2001-05-01 00:00:00
                                a3|test2|2001-05-01 00:00:00|2001-07-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.rast.list", quiet=True, flags="s",  input="A@test2")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test3|2001-01-01 00:00:00|2001-04-01 00:00:00
                                a2|test3|2001-04-01 00:00:00|2001-07-01 00:00:00
                                a3|test3|2001-07-01 00:00:00|2001-10-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.rast.list", quiet=True, flags="s",  input="A@test3")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test4|2001-01-01 00:00:00|2001-05-01 00:00:00
                                a2|test4|2001-05-01 00:00:00|2001-09-01 00:00:00
                                a3|test4|2001-09-01 00:00:00|2002-01-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.rast.list", quiet=True, flags="s",  input="A@test4")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test5|2001-01-01 00:00:00|2001-06-01 00:00:00
                                a2|test5|2001-06-01 00:00:00|2001-11-01 00:00:00
                                a3|test5|2001-11-01 00:00:00|2002-04-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.rast.list", quiet=True, flags="s",  input="A@test5")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

    def test_strds_info(self):  
        self.runModule("g.mapset", mapset="test4")
        tinfo_string="""id=A@test1
                                    name=A
                                    mapset=test1
                                    start_time=2001-01-01 00:00:00
                                    end_time=2001-04-01 00:00:00
                                    granularity=1 month"""

        info = SimpleModule("t.info", flags="g", input="A@test1")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")
  
        self.runModule("g.mapset", mapset="test3")
        tinfo_string="""id=A@test2
                                    name=A
                                    mapset=test2
                                    start_time=2001-01-01 00:00:00
                                    end_time=2001-07-01 00:00:00
                                    granularity=2 months"""

        info = SimpleModule("t.info", flags="g", input="A@test2")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")
  
        self.runModule("g.mapset", mapset="test2")
        tinfo_string="""id=A@test3
                                    name=A
                                    mapset=test3
                                    start_time=2001-01-01 00:00:00
                                    end_time=2001-10-01 00:00:00
                                    granularity=3 months"""

        info = SimpleModule("t.info", flags="g", input="A@test3")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")
  
        self.runModule("g.mapset", mapset="test1")
        tinfo_string="""id=A@test4
                                    name=A
                                    mapset=test4
                                    start_time=2001-01-01 00:00:00
                                    end_time=2002-01-01 00:00:00
                                    granularity=4 months"""

        info = SimpleModule("t.info", flags="g", input="A@test4")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=A@test5
                                    name=A
                                    mapset=test5
                                    start_time=2001-01-01 00:00:00
                                    end_time=2002-04-01 00:00:00
                                    granularity=5 months"""

        info = SimpleModule("t.info", flags="g", input="A@test5")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_raster_info(self):  
        self.runModule("g.mapset", mapset="test3")
        tinfo_string="""id=a1@test1
                                name=a1
                                mapset=test1
                                creator=soeren
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-02-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="raster",  input="a1@test1")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=a1@test2
                                name=a1
                                mapset=test2
                                creator=soeren
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-03-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="raster",  input="a1@test2")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=a1@test3
                                name=a1
                                mapset=test3
                                creator=soeren
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-04-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="raster",  input="a1@test3")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=a1@test4
                                name=a1
                                mapset=test4
                                creator=soeren
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-05-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="raster",  input="a1@test4")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=a1@test5
                                name=a1
                                mapset=test5
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-06-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="raster",  input="a1@test5")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
