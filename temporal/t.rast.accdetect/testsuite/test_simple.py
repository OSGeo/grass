"""test distributed temporal databases with stvds

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Luca Delucchi
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
import os


class TestRasterExtraction(TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,
                      t=50,  res=10,  res3=10)
        for i in range(1,101):
            cls.runModule("r.mapcalc", flags='s',  overwrite=True,
                          expression="a_mapcalc{nu} = rand(1,10)".format(nu=i))

        cls.runModule("t.create", type="strds", temporaltype="absolute",
                      output="A", title="A test", description="A test",
                      overwrite=True)
        glist = SimpleModule('g.list', type='raster', pattern="a_mapcalc*",
                             separator=',')
        cls.runModule(glist, expecting_stdout=True)
        maps = glist.outputs.stdout.strip()
        cls.runModule("t.register", flags="i",  type="raster",  input="A",
                      maps=maps.strip(), increment="1 month",
                      start="2001-01-01 00:00:00", overwrite=True)
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", type="strds", inputs="A")

    def tearDown(self):
        """Remove generated data"""
        self.runModule("t.remove", flags="rf", type="strds", inputs="B")
        self.runModule("t.remove", flags="rf", type="strds", inputs="C")
        
    def test_simple(self):
        self.assertModule('t.rast.accdetect', input='A', occurrence='B',
                          indicator="C", start="2001-01-01", cycle="12 months",
                          basename='result', range=(1,8))
        tinfo_string="""semantic_type=mean
        start_time='2001-01-01 00:00:00'
        end_time='2009-05-01 00:00:00'
        granularity='1 month'
        map_time=interval
        number_of_maps=100"""
        info = SimpleModule("t.info", flags="g", type="strds", input="B")
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

        tinfo_string="""semantic_type=mean
        start_time='2001-01-01 00:00:00'
        end_time='2009-05-01 00:00:00'
        granularity='1 month'
        map_time=interval
        number_of_maps=100"""
        info = SimpleModule("t.info", flags="g", type="strds", input="C")
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_stop(self):
        self.assertModule('t.rast.accdetect', input='A', occurrence='B',
                          indicator="C", start="2001-01-01", stop='2008-12-31',
                          cycle="12 months", basename='result', range=(1,8))
        tinfo_string="""semantic_type=mean
        start_time='2001-01-01 00:00:00'
        end_time='2009-01-01 00:00:00'
        granularity='1 month'
        map_time=interval
        number_of_maps=96"""
        info = SimpleModule("t.info", flags="g", type="strds", input="B")
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

        tinfo_string="""semantic_type=mean
        start_time='2001-01-01 00:00:00'
        end_time='2009-01-01 00:00:00'
        granularity='1 month'
        map_time=interval
        number_of_maps=96"""
        info = SimpleModule("t.info", flags="g", type="strds", input="C")
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_time_suffix(self):
        self.assertModule('t.rast.accdetect', input='A', occurrence='B',
                          indicator="C", start="2001-01-01", cycle="12 months", suffix='time',
                          basename='result', range=(1,8))
        self.assertRasterDoesNotExist('result_2001_01')
        self.assertRasterExists('result_2001_01_01T00_00_00')
        self.assertRasterDoesNotExist('result_indicator_2001_01')
        self.assertRasterExists('result_indicator_2001_01_01T00_00_00')

    def test_num_suffix(self):
        self.assertModule('t.rast.accdetect', input='A', occurrence='B',
                          indicator="C", start="2001-01-01", cycle="12 months",
                          suffix='count%03', basename='result', range=(1,8))
        self.assertRasterDoesNotExist('result_2001_01')
        self.assertRasterExists('result_001')
        self.assertRasterDoesNotExist('result_00001')

        self.assertRasterDoesNotExist('result_indicator_2001_01')
        self.assertRasterExists('result_indicator_001')
        self.assertRasterDoesNotExist('result_indicator_00001')

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
