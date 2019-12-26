# -*- coding: utf-8 -*-
"""Test v.what.strds

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Luca Delucchi
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
import grass.script as gscript
from grass.script.utils import decode


class TestWhatStrds(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50,
                      res=10, res3=10)
        cls.runModule("r.mapcalc", expression="a_1 = 100", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_2 = 200", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_3 = 300", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_4 = 400", overwrite=True)

        cls.runModule("v.random", output="points", npoints=3, seed=1,
                      overwrite=True)

        cls.runModule("t.create", type="strds", temporaltype="absolute",
                      output="A", title="A test", description="A test",
                      overwrite=True)
        cls.runModule("t.register", flags="i", type="raster", input="A",
                      maps="a_1,a_2,a_3,a_4", start="2001-01-01",
                      increment="3 months", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", type="strds", inputs="A")
        cls.del_temp_region()

    def test_output(self):
        self.assertModule("v.what.strds", input="points", strds="A",
                          output="what_strds", overwrite=True)

        maps = gscript.list_strings('vector')
        self.assertIn('what_strds@{ma}'.format(ma=gscript.gisenv()['MAPSET']),
                      maps)

    def test_values(self):
        self.assertModule("v.what.strds", input="points", strds="A",
                          output="what_strds", overwrite=True)
        db_sel = SimpleModule("v.db.select", map="what_strds")
        self.assertModule(db_sel)
        output = """cat|A_2001_01_01|A_2001_04_01|A_2001_07_01|A_2001_10_01
1|100|200|300|400
2|100|200|300|400
3|100|200|300|400
"""
        self.assertMultiLineEqual(output, decode(db_sel.outputs.stdout))

if __name__ == '__main__':
    test()
