"""Test t.rast.univar

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestRasterUnivar(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=1,  res3=1)

        cls.runModule("r3.mapcalc", expression="a_1 = 100",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a_2 = 200",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a_3 = 300",  overwrite=True)
        cls.runModule("r3.mapcalc", expression="a_4 = 400",  overwrite=True)

        cls.runModule("t.create",  type="str3ds",  temporaltype="absolute",
                                   output="A",  title="A test",  description="A test",
                                   overwrite=True)
        cls.runModule("t.register",  flags="i",  type="raster_3d",  input="A",
                                     maps="a_1,a_2,a_3,a_4",  start="2001-01-01", 
                                     increment="3 months",  overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove",  flags="rf",  type="str3ds",
                                   inputs="A")
        cls.del_temp_region()

    def test_1(self):
        
        t_rast3d_univar = SimpleModule("t.rast3d.univar", input="A",
                                                          where="start_time >= '2001-01-01'",
                                                          overwrite=True, verbose=True)
        self.assertModule(t_rast3d_univar)

        univar_text=u"""id|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_1@testing|2001-01-01 00:00:00|2001-04-01 00:00:00|100|100|100|100|0|0|0|48000000|0|480000|480000
a_2@testing|2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|96000000|0|480000|480000
a_3@testing|2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|144000000|0|480000|480000
a_4@testing|2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|192000000|0|480000|480000
"""
        for ref, res in zip(univar_text.split("\n"), t_rast3d_univar.outputs.stdout.split("\n")):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line,  res_line)

    def test_2(self):

        t_rast3d_univar = SimpleModule("t.rast3d.univar", input="A",
                                                          where="start_time >= '2001-03-01'",
                                                          overwrite=True, verbose=True)
        self.assertModule(t_rast3d_univar)

        univar_text=u"""id|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_2@testing|2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|96000000|0|480000|480000
a_3@testing|2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|144000000|0|480000|480000
a_4@testing|2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|192000000|0|480000|480000
"""
        for ref, res in zip(univar_text.split("\n"), t_rast3d_univar.outputs.stdout.split("\n")):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line,  res_line)

    def test_3(self):

        self.assertModule("t.rast3d.univar", input="A",
                                             output="univar_output.txt",
                                             where="start_time >= '2001-03-01'",
                                             overwrite=True, verbose=True)

        univar_text=u"""id|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_2@testing|2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|96000000|0|480000|480000
a_3@testing|2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|144000000|0|480000|480000
a_4@testing|2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|192000000|0|480000|480000
"""
        univar_output = open("univar_output.txt", "r").read()

        for ref, res in zip(univar_text.split("\n"), univar_output.split("\n")):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line,  res_line)

    def test_4(self):

        self.assertModule("t.rast3d.univar", input="A",
                                             output="univar_output.txt", flags="s",
                                             where="start_time >= '2001-03-01'",
                                             overwrite=True, verbose=True)

        univar_text=u"""a_2@testing|2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|96000000|0|480000|480000
a_3@testing|2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|144000000|0|480000|480000
a_4@testing|2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|192000000|0|480000|480000
"""
        univar_output = open("univar_output.txt", "r").read()

        for ref, res in zip(univar_text.split("\n"), univar_output.split("\n")):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line,  res_line)

    def test_5_error_handling_empty_strds(self):
        # Empty str3ds
        self.assertModuleFail("t.rast3d.univar", input="A",
                                                 output="univar_output.txt",
                                                 where="start_time >= '2015-03-01'",
                                                 overwrite=True, verbose=True)

    def test_6_error_handling_no_input(self):
        # No input
        self.assertModuleFail("t.rast3d.univar",  output="out.txt")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
