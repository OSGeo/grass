"""test distributed temporal databases with stvds

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

mapset_count = 0
class TestRasterExtraction(TestCase):

    @classmethod
    def setUpClass(cls):
       os.putenv("GRASS_OVERWRITE", "1")
       for i in range(1, 5): 
            cls.runModule("g.mapset", flags="c", mapset="testvect%i"%i)
            cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)
            # Use always the current mapset as temporal database
            cls.runModule("v.random", output="a1",  npoints=20)
            cls.runModule("v.random", output="a2",  npoints=20)
            cls.runModule("v.random", output="a3",  npoints=20)
            
            cls.runModule("t.create",  type="stvds",  temporaltype="absolute",  
                                         output="A",  title="A testvect",  description="A testvect")
            cls.runModule("t.register",  flags="i",  type="vector",  input="A",  
                                         maps="a1,a2,a3",  
                                         start="2001-01-01", increment="%i months"%i)

    def test_tlist(self):      
        self.runModule("g.mapset", mapset="testvect1")
        
        list_string = """A|testvect1|2001-01-01 00:00:00|2001-04-01 00:00:00|3
                                A|testvect2|2001-01-01 00:00:00|2001-07-01 00:00:00|3
                                A|testvect3|2001-01-01 00:00:00|2001-10-01 00:00:00|3
                                A|testvect4|2001-01-01 00:00:00|2002-01-01 00:00:00|3"""
                                
        entries = list_string.split("\n")
        
        t_list = SimpleModule("t.list", quiet=True, 
                                           columns=["name","mapset,start_time","end_time","number_of_maps"],  
                                           type="stvds",  where='name = "A"')
        self.assertModule(t_list)
        
        out = t_list.outputs["stdout"].value
        
        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())
        
    def test_tvect_list(self):      
        self.runModule("g.mapset", mapset="testvect1")

        list_string = """a1|testvect1|2001-01-01 00:00:00|2001-02-01 00:00:00
                                a2|testvect1|2001-02-01 00:00:00|2001-03-01 00:00:00
                                a3|testvect1|2001-03-01 00:00:00|2001-04-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.vect.list", quiet=True, flags="s",  
                                  columns=["name","mapset","start_time","end_time"] , 
                                  input="A@testvect1")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|testvect2|2001-01-01 00:00:00|2001-03-01 00:00:00
                                a2|testvect2|2001-03-01 00:00:00|2001-05-01 00:00:00
                                a3|testvect2|2001-05-01 00:00:00|2001-07-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.vect.list", quiet=True, flags="s", 
                                  columns=["name","mapset","start_time","end_time"] , 
                                  input="A@testvect2")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|testvect3|2001-01-01 00:00:00|2001-04-01 00:00:00
                                a2|testvect3|2001-04-01 00:00:00|2001-07-01 00:00:00
                                a3|testvect3|2001-07-01 00:00:00|2001-10-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.vect.list", quiet=True, flags="s", 
                                  columns=["name","mapset","start_time","end_time"] , 
                                  input="A@testvect3")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|testvect4|2001-01-01 00:00:00|2001-05-01 00:00:00
                                a2|testvect4|2001-05-01 00:00:00|2001-09-01 00:00:00
                                a3|testvect4|2001-09-01 00:00:00|2002-01-01 00:00:00"""

        entries = list_string.split("\n")

        trast_list = SimpleModule("t.vect.list", quiet=True, flags="s",
                                  columns=["name","mapset","start_time","end_time"] , 
                                  input="A@testvect4")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a,  b in zip(list_string.split("\n"),  out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

    def test_stvds_info(self):  
        self.runModule("g.mapset", mapset="testvect4")
        tinfo_string="""id=A@testvect1
                                    name=A
                                    mapset=testvect1
                                    start_time=2001-01-01 00:00:00
                                    end_time=2001-04-01 00:00:00
                                    granularity=1 month"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="A@testvect1")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")
  
        self.runModule("g.mapset", mapset="testvect3")
        tinfo_string="""id=A@testvect2
                                    name=A
                                    mapset=testvect2
                                    start_time=2001-01-01 00:00:00
                                    end_time=2001-07-01 00:00:00
                                    granularity=2 months"""

        info = SimpleModule("t.info", flags="g", type="stvds",  input="A@testvect2")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")
  
        self.runModule("g.mapset", mapset="testvect2")
        tinfo_string="""id=A@testvect3
                                    name=A
                                    mapset=testvect3
                                    start_time=2001-01-01 00:00:00
                                    end_time=2001-10-01 00:00:00
                                    granularity=3 months"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="A@testvect3")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")
  
        self.runModule("g.mapset", mapset="testvect1")
        tinfo_string="""id=A@testvect4
                                    name=A
                                    mapset=testvect4
                                    start_time=2001-01-01 00:00:00
                                    end_time=2002-01-01 00:00:00
                                    granularity=4 months"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="A@testvect4")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def testv_vector_info(self):  
        self.runModule("g.mapset", mapset="testvect3")
        tinfo_string="""id=a1@testvect1
                                name=a1
                                mapset=testvect1
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-02-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="vector",  input="a1@testvect1")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=a1@testvect2
                                name=a1
                                mapset=testvect2
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-03-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="vector",  input="a1@testvect2")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=a1@testvect3
                                name=a1
                                mapset=testvect3
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-04-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="vector",  input="a1@testvect3")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string="""id=a1@testvect4
                                name=a1
                                mapset=testvect4
                                temporal_type=absolute
                                start_time=2001-01-01 00:00:00
                                end_time=2001-05-01 00:00:00 """

        info = SimpleModule("t.info", flags="g", type="vector",  input="a1@testvect4")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
