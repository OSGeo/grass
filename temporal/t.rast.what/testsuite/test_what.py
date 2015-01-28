"""Test t.rast.what

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
                          nprocs=1,  overwrite=True)

        self.assertFileMd5("out_row.txt", "55209718566d70e1427bd1fecf844d53")

    def test_col_output(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_col.txt", 
                          points="points", flags="n",
                          layout="col",
                          nprocs=1,  overwrite=True)

        self.assertFileMd5("out_col.txt", "825f73e284c0f6e09cf873bd3a1bf15f")

    def test_timerow_output(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_timerow.txt", 
                          points="points", flags="n",
                          layout="timerow",
                          nprocs=1,  overwrite=True)

        self.assertFileMd5("out_timerow.txt", "129fe0b63019e505232efa20ad42c03a")

    def test_row_output_where_parallel(self):
        self.assertModule("t.rast.what",  strds="A",  output="out_where.txt", 
                          points="points", flags="n",
                          where="start_time > '2001-03-01'",  
                          nprocs=4,  overwrite=True)

        self.assertFileMd5("out_where.txt", "af731bec01fedc262f4ac162fe420707")

    def test_empty_strds(self):
        self.assertModuleFail("t.rast.what",  strds="A",  output="out_error.txt", 
                          points="points", flags="n",
                          where="start_time > '2002-03-01'",  
                          nprocs=4,  overwrite=True)


class TestRasterWhatFails(TestCase):

    def test_error_handling(self):
        # No vector map, no strds
        self.assertModuleFail("t.rast.what",  output="out.txt")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
