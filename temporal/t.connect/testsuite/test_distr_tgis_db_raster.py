"""test distributed temporal databases with strds

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.utils import silent_rmtree
import os


class TestRasterExtraction(TestCase):

    mapsets_to_remove = []
    outfile = 'rastlist.txt'
    gisenv = SimpleModule('g.gisenv', get='MAPSET')
    TestCase.runModule(gisenv, expecting_stdout=True)
    old_mapset = gisenv.outputs.stdout.strip()

    @classmethod
    def setUpClass(cls):
        os.putenv("GRASS_OVERWRITE", "1")
        for i in range(1, 7):
            mapset_name = "test%i" % i
            cls.runModule("g.mapset", flags="c", mapset=mapset_name)
            cls.mapsets_to_remove.append(mapset_name)
            cls.runModule("g.region", s=0, n=80,
                          w=0, e=120, b=0, t=50, res=10, res3=10)
            cls.runModule("t.connect", flags="d")
            cls.runModule("t.info", flags="d")
            cls.runModule("r.mapcalc", expression="a1 = 100")
            cls.runModule("r.mapcalc", expression="a2 = 200")
            cls.runModule("r.mapcalc", expression="a3 = 300")
            cls.runModule("t.create", type="strds", temporaltype="absolute",
                          output="A", title="A test", description="A test")
            cls.runModule("t.register", flags="i", type="raster", input="A",
                          maps="a1,a2,a3",
                          start="2001-01-01", increment="%i months" % i)

        # Add the new mapsets to the search path
        for mapset in cls.mapsets_to_remove:
            cls.runModule("g.mapset", mapset=mapset)
            cls.runModule("g.mapsets", operation="add", mapset=','.join(cls.mapsets_to_remove))


    @classmethod
    def tearDownClass(cls):
        gisenv = SimpleModule('g.gisenv', get='GISDBASE')
        cls.runModule(gisenv, expecting_stdout=True)
        gisdbase = gisenv.outputs.stdout.strip()
        gisenv = SimpleModule('g.gisenv', get='LOCATION_NAME')
        cls.runModule(gisenv, expecting_stdout=True)
        location = gisenv.outputs.stdout.strip()
        cls.runModule("g.mapset", mapset=cls.old_mapset)
        for mapset_name in cls.mapsets_to_remove:
            mapset_path = os.path.join(gisdbase, location, mapset_name)
            silent_rmtree(mapset_path)

    def test_tlist(self):
        self.runModule("g.mapset", mapset="test1")

        list_string = """A|test1|2001-01-01 00:00:00|2001-04-01 00:00:00|3
                                A|test2|2001-01-01 00:00:00|2001-07-01 00:00:00|3
                                A|test3|2001-01-01 00:00:00|2001-10-01 00:00:00|3
                                A|test4|2001-01-01 00:00:00|2002-01-01 00:00:00|3
                                A|test5|2001-01-01 00:00:00|2002-04-01 00:00:00|3
                                A|test6|2001-01-01 00:00:00|2002-07-01 00:00:00|3"""

        t_list = SimpleModule(
            "t.list", quiet=True,
            columns=["name", "mapset,start_time", "end_time", "number_of_maps"],
            type="strds", where='name = "A"')
        self.assertModule(t_list)

        out = t_list.outputs["stdout"].value

        for a, b in zip(list_string.split("\n"), out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        t_list = SimpleModule(
            "t.list", quiet=True,
            columns=["name", "mapset,start_time", "end_time", "number_of_maps"],
            type="strds", where='name = "A"', output=self.outfile)
        self.assertModule(t_list)
        self.assertFileExists(self.outfile)
        with open(self.outfile, 'r') as f:
            read_data = f.read()
        for a, b in zip(list_string.split("\n"), read_data.split("\n")):
            self.assertEqual(a.strip(), b.strip())
        #self.assertLooksLike(reference=read_data, actual=list_string)
        if os.path.isfile(self.outfile):
            os.remove(self.outfile)

    def test_trast_list(self):
        self.runModule("g.mapset", mapset="test1")

        list_string = """a1|test1|2001-01-01 00:00:00|2001-02-01 00:00:00
                                a2|test1|2001-02-01 00:00:00|2001-03-01 00:00:00
                                a3|test1|2001-03-01 00:00:00|2001-04-01 00:00:00"""

        trast_list = SimpleModule(
            "t.rast.list", quiet=True, flags="u", input="A@test1")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a, b in zip(list_string.split("\n"), out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test2|2001-01-01 00:00:00|2001-03-01 00:00:00
                                a2|test2|2001-03-01 00:00:00|2001-05-01 00:00:00
                                a3|test2|2001-05-01 00:00:00|2001-07-01 00:00:00"""

        trast_list = SimpleModule(
            "t.rast.list", quiet=True, flags="u", input="A@test2")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a, b in zip(list_string.split("\n"), out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test3|2001-01-01 00:00:00|2001-04-01 00:00:00
                                a2|test3|2001-04-01 00:00:00|2001-07-01 00:00:00
                                a3|test3|2001-07-01 00:00:00|2001-10-01 00:00:00"""

        trast_list = SimpleModule(
            "t.rast.list", quiet=True, flags="u", input="A@test3")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a, b in zip(list_string.split("\n"), out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test4|2001-01-01 00:00:00|2001-05-01 00:00:00
                                a2|test4|2001-05-01 00:00:00|2001-09-01 00:00:00
                                a3|test4|2001-09-01 00:00:00|2002-01-01 00:00:00"""

        trast_list = SimpleModule(
            "t.rast.list", quiet=True, flags="u", input="A@test4")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a, b in zip(list_string.split("\n"), out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        list_string = """a1|test5|2001-01-01 00:00:00|2001-06-01 00:00:00
                                a2|test5|2001-06-01 00:00:00|2001-11-01 00:00:00
                                a3|test5|2001-11-01 00:00:00|2002-04-01 00:00:00"""

        trast_list = SimpleModule(
            "t.rast.list", quiet=True, flags="u", input="A@test5")
        self.assertModule(trast_list)

        out = trast_list.outputs["stdout"].value

        for a, b in zip(list_string.split("\n"), out.split("\n")):
            self.assertEqual(a.strip(), b.strip())

        trast_list = SimpleModule("t.rast.list", quiet=True, flags="u",
                                  input="A@test5", output=self.outfile)
        self.assertModule(trast_list)
        self.assertFileExists(self.outfile)
        with open(self.outfile, 'r') as f:
            read_data = f.read()
        for a, b in zip(list_string.split("\n"), read_data.split("\n")):
            self.assertEqual(a.strip(), b.strip())
        if os.path.isfile(self.outfile):
            os.remove(self.outfile)

    def test_strds_info(self):
        self.runModule("g.mapset", mapset="test4")
        tinfo_string = """id=A@test1
                                    name=A
                                    mapset=test1
                                    start_time='2001-01-01 00:00:00'
                                    end_time='2001-04-01 00:00:00'
                                    granularity='1 month'"""

        info = SimpleModule("t.info", flags="g", input="A@test1")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        self.runModule("g.mapset", mapset="test3")
        tinfo_string = """id=A@test2
                                    name=A
                                    mapset=test2
                                    start_time='2001-01-01 00:00:00'
                                    end_time='2001-07-01 00:00:00'
                                    granularity='2 months'"""

        info = SimpleModule("t.info", flags="g", input="A@test2")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        self.runModule("g.mapset", mapset="test2")
        tinfo_string = """id=A@test3
                                    name=A
                                    mapset=test3
                                    start_time='2001-01-01 00:00:00'
                                    end_time='2001-10-01 00:00:00'
                                    granularity='3 months'"""

        info = SimpleModule("t.info", flags="g", input="A@test3")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        self.runModule("g.mapset", mapset="test1")
        tinfo_string = """id=A@test4
                                    name=A
                                    mapset=test4
                                    start_time='2001-01-01 00:00:00'
                                    end_time='2002-01-01 00:00:00'
                                    granularity='4 months'"""

        info = SimpleModule("t.info", flags="g", input="A@test4")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string = """id=A@test5
                                    name=A
                                    mapset=test5
                                    start_time='2001-01-01 00:00:00'
                                    end_time='2002-04-01 00:00:00'
                                    granularity='5 months'"""

        info = SimpleModule("t.info", flags="g", input="A@test5")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

    def test_raster_info(self):
        self.runModule("g.mapset", mapset="test3")
        tinfo_string = """id=a1@test1
                                name=a1
                                mapset=test1
                                temporal_type=absolute
                                start_time='2001-01-01 00:00:00'
                                end_time='2001-02-01 00:00:00'"""

        info = SimpleModule(
            "t.info", flags="g", type="raster", input="a1@test1")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string = """id=a1@test2
                                name=a1
                                mapset=test2
                                temporal_type=absolute
                                start_time='2001-01-01 00:00:00'
                                end_time='2001-03-01 00:00:00'"""

        info = SimpleModule(
            "t.info", flags="g", type="raster", input="a1@test2")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string = """id=a1@test3
                                name=a1
                                mapset=test3
                                temporal_type=absolute
                                start_time='2001-01-01 00:00:00'
                                end_time='2001-04-01 00:00:00'"""

        info = SimpleModule(
            "t.info", flags="g", type="raster", input="a1@test3")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string = """id=a1@test4
                                name=a1
                                mapset=test4
                                temporal_type=absolute
                                start_time='2001-01-01 00:00:00'
                                end_time='2001-05-01 00:00:00'"""

        info = SimpleModule(
            "t.info", flags="g", type="raster", input="a1@test4")
        self.assertModuleKeyValue(
            module=info, reference=tinfo_string, precision=2, sep="=")

        tinfo_string = """id=a1@test5
                                name=a1
                                mapset=test5
                                temporal_type=absolute
                                start_time='2001-01-01 00:00:00'
                                end_time='2001-06-01 00:00:00'"""

        info = SimpleModule(
            "t.info", flags="g", type="raster", input="a1@test5")


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
