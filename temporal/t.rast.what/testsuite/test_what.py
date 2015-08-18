"""Test t.rast.what

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

class TestRasterWhat(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region",  s=0,  n=80,  w=0,  e=120,  b=0,  t=50,  res=10,  res3=10)

        cls.runModule("r.mapcalc", expression="a_1 = 100",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_2 = 200",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_3 = 300",  overwrite=True)
        cls.runModule("r.mapcalc", expression="a_4 = 400",  overwrite=True)
        
        cls.runModule("v.random", output="points", npoints=3, seed=1, overwrite=True)

        cls.runModule("t.create",  type="strds",  temporaltype="absolute",  
                                 output="A",  title="A test",  description="A test",  
                                 overwrite=True)
        cls.runModule("t.register",  flags="i",  type="raster",  input="A",  
                                     maps="a_1,a_2,a_3,a_4",  start="2001-01-01", 
                                     increment="3 months",  overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove",  flags="rf",  type="strds",  
                                   inputs="A")
        cls.del_temp_region()

    def test_row_output(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_row.txt", 
                          points="points", flags="n",
                          layout="row",
                          nprocs=1,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_row.txt", "55209718566d70e1427bd1fecf844d53", text=True)


    def test_row_output_coords(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_row_coords.txt", 
                          coordinates=(30,30, 45, 45), flags="n",
                          layout="row",
                          nprocs=1,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_row_coords.txt", "cd917ac4848786f1b944512eed1da5bc", text=True)


    def test_row_output_coords_stdin(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_row_coords.txt", 
                          flags="ni",
                          layout="row", stdin_="30 30\n45 45",
                          nprocs=1,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_row_coords.txt", "cd917ac4848786f1b944512eed1da5bc", text=True)

    def test_col_output(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_col.txt", 
                          points="points", flags="n",
                          layout="col",
                          nprocs=1,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_col.txt", "8720cc237b46ddb18f11440469d0e13f", text=True)

    def test_col_output_coords(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_col_coords.txt", 
                          coordinates=(30,30, 45, 45), flags="n",
                          layout="col",
                          nprocs=1,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_col_coords.txt", "ac44ebc5aa040d4ce2a5787e95f208ec", text=True)

    def test_timerow_output(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_timerow.txt", 
                          points="points", flags="n",
                          layout="timerow",
                          nprocs=1,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_timerow.txt", "129fe0b63019e505232efa20ad42c03a", text=True)

    def test_timerow_output_coords(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_timerow_coords.txt", 
                          coordinates=(30,30, 45, 45), flags="n",
                          layout="timerow",
                          nprocs=1,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_timerow_coords.txt", "ca4ee0e7e4aaca170d6034e0d57d292d", text=True)

    def test_row_stdout_where_parallel(self):
        
        t_rast_what = SimpleModule("t.rast.what",  strds="A",  output="-", 
                                                      points="points", flags="n",
                                                      where="start_time > '2001-03-01'",  
                                                      nprocs=4,  overwrite=True,  verbose=True)
        self.assertModule(t_rast_what)

        text="""x|y|start|end|value
115.0043586274|36.3593955783|2001-04-01 00:00:00|2001-07-01 00:00:00|200
79.6816763826|45.2391522853|2001-04-01 00:00:00|2001-07-01 00:00:00|200
97.4892579600|79.2347263950|2001-04-01 00:00:00|2001-07-01 00:00:00|200
115.0043586274|36.3593955783|2001-07-01 00:00:00|2001-10-01 00:00:00|300
79.6816763826|45.2391522853|2001-07-01 00:00:00|2001-10-01 00:00:00|300
97.4892579600|79.2347263950|2001-07-01 00:00:00|2001-10-01 00:00:00|300
115.0043586274|36.3593955783|2001-10-01 00:00:00|2002-01-01 00:00:00|400
79.6816763826|45.2391522853|2001-10-01 00:00:00|2002-01-01 00:00:00|400
97.4892579600|79.2347263950|2001-10-01 00:00:00|2002-01-01 00:00:00|400
"""
        self.assertLooksLike(text,  t_rast_what.outputs.stdout)

    def test_row_stdout_where_parallel2(self):
        """Here without output definition, the default is used then"""
        
        t_rast_what = SimpleModule("t.rast.what",  strds="A",  
                                                      points="points", flags="n",
                                                      where="start_time > '2001-03-01'",  
                                                      nprocs=4,  overwrite=True,  verbose=True)
        self.assertModule(t_rast_what)

        text="""x|y|start|end|value
115.0043586274|36.3593955783|2001-04-01 00:00:00|2001-07-01 00:00:00|200
79.6816763826|45.2391522853|2001-04-01 00:00:00|2001-07-01 00:00:00|200
97.4892579600|79.2347263950|2001-04-01 00:00:00|2001-07-01 00:00:00|200
115.0043586274|36.3593955783|2001-07-01 00:00:00|2001-10-01 00:00:00|300
79.6816763826|45.2391522853|2001-07-01 00:00:00|2001-10-01 00:00:00|300
97.4892579600|79.2347263950|2001-07-01 00:00:00|2001-10-01 00:00:00|300
115.0043586274|36.3593955783|2001-10-01 00:00:00|2002-01-01 00:00:00|400
79.6816763826|45.2391522853|2001-10-01 00:00:00|2002-01-01 00:00:00|400
97.4892579600|79.2347263950|2001-10-01 00:00:00|2002-01-01 00:00:00|400
"""
        self.assertLooksLike(text,  t_rast_what.outputs.stdout)

    def test_row_output_where_parallel(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_where.txt", 
                          points="points", flags="n",
                          where="start_time > '2001-03-01'",  
                          nprocs=4,  overwrite=True,  verbose=True)

        self.assertFileMd5("out_where.txt", "af731bec01fedc262f4ac162fe420707", text=True)

    def test_empty_strds(self):
        self.assertModuleFail("t.rast.what",  strds="A",  output="out_error.txt", 
                          points="points", flags="n",
                          where="start_time > '2002-03-01'",  
                          nprocs=4,  overwrite=True,  verbose=True)


class TestRasterWhatFails(TestCase):

    def test_error_handling(self):
        # No vector map, no strds, no coordinates
        self.assertModuleFail("t.rast.what",  output="out.txt")
        # No vector map, no coordinates
        self.assertModuleFail("t.rast.what",  strds="A",  output="out.txt")
        # Points and coordinates are mutually exclusive
        self.assertModuleFail("t.rast.what",  points="points", coordinates=(30, 30, 45, 45),  output="out.txt",  strds="A")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
