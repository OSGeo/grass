"""Test t.rast.contour

(C) 2015 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestRasterContour(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

        cls.runModule("r.mapcalc", expression="a_1 = row()", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_2 = col()", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_3 = row()", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_4 = col()", overwrite=True)
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

    def test_register_empty_maps(self):
        self.assertModule("t.rast.contour", input="A", output="result",
                          levels=(1,2,3,4,5,6,7,8), flags="n",
                          basename="test",
                          nprocs=1, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-04-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=5
                        lines=28
                        primitives=28
                        nodes=56"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_simple(self):
        """Do not register empty maps"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          levels=(1,2,3,4,5,6,7,8),
                          basename="test",
                          nprocs=1, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-01-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=4
                        lines=28
                        primitives=28
                        nodes=56"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_where(self):
        """Use where statement and do not register empty maps"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          levels=(1,2,3,4,5,6,7,8),
                          basename="test",
                          where="start_time > '2001-02-01'",
                          nprocs=1, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-04-01 00:00:00'
                        end_time='2002-01-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=3
                        lines=21
                        primitives=21
                        nodes=42"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_parallel(self):
        """Run 4 contour processes do not create attribute tables"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          levels=(1,2,3,4,5,6,7,8),
                          basename="test", flags="t",
                          nprocs=4, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-01-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=4
                        lines=28
                        primitives=28
                        nodes=56"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_parallel_cut(self):
        """Do not register empty maps"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          levels=(1,2,3,4,5,6,7,8), cut=2,
                          basename="test", flags="t",
                          nprocs=4, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-01-01 00:00:00'
                        end_time='2002-01-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=4
                        lines=28
                        primitives=28
                        nodes=56"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")

    def test_where_step(self):
        """Use where statement and do not register empty maps"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          step=1, minlevel=1, maxlevel=8,
                          basename="test",
                          where="start_time > '2001-02-01'",
                          nprocs=1, overwrite=True, verbose=True)

        #self.assertModule("t.info",  type="stvds", flags="g",  input="result")

        tinfo_string="""start_time='2001-04-01 00:00:00'
                        end_time='2002-01-01 00:00:00'
                        granularity='3 months'
                        map_time=interval
                        number_of_maps=3
                        lines=21
                        primitives=21
                        nodes=42"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="result")
        self.assertModuleKeyValue(module=info, reference=tinfo_string, precision=2, sep="=")


    def test_suffix_num(self):
        """Test the -s flag"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          step=1, minlevel=1, maxlevel=8,
                          basename="time", suffix='num%03',
                          where="start_time > '2001-02-01'",
                          nprocs=1, overwrite=True, verbose=True)

        self.assertVectorExists('time_001')
        self.assertVectorDoesNotExist('time_00005')
        self.assertVectorDoesNotExist('time_2001_07')

    def test_suffix_time(self):
        """Test the -s flag"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          step=1, minlevel=1, maxlevel=8,
                          basename="time", suffix='time',
                          where="start_time > '2001-02-01'",
                          nprocs=1, overwrite=True, verbose=True)

        self.assertVectorExists('time_2001_07_01T00_00_00')
        self.assertVectorDoesNotExist('time_00005')
        self.assertVectorDoesNotExist('time_2001_07')


class TestRasterContourFails(TestCase):
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
        # No basename
        self.assertModuleFail("t.rast.contour", input="A", output="result",
                              step=1, minlevel=1, maxlevel=8,
                              where="start_time > '2001-02-01'",
                              nprocs=1, overwrite=True, verbose=True)


    def test_empty_strds(self):
        """Test for empty strds"""
        self.assertModule("t.rast.contour", input="A", output="result",
                          basename="test",
                          step=1, minlevel=1, maxlevel=8,
                          where="start_time > '2010-01-01'",
                          nprocs=1, overwrite=True, verbose=True)

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
