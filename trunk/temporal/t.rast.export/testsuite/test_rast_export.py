"""
Test t.rast.export

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author: lucadelu
"""

from grass.gunittest.case import TestCase
import grass.script as gscript
import os

class TestRasterExport(TestCase):

    tmp = gscript.tempdir()
    float_ = os.path.join(tmp, "geotiffloat")
    int_ = os.path.join(tmp, "geotifint")
    grid = os.path.join(tmp, "grid")
    pack = os.path.join(tmp, "pack")

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10,
                      res3=10)
        maps = []
        for i in range(10):
            cls.runModule("r.mapcalc", expression="a_{id_} = rand(0.1,1.0)".format(id_=i),
                          flags="s", overwrite=True)
            maps.append("a_{id_}".format(id_=i))

        cls.runModule("t.create", type="strds", temporaltype="absolute",  
                      output="A", title="A test", description="A test",
                      overwrite=True)
        cls.runModule("t.register", flags="i", type="raster", input="A",  
                      maps=maps, start="2001-01-01",
                      increment="4 months", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", inputs="A")

    def test_simple_geotif(self):
        self.assertModule("t.rast.export", input="A", output=self.float_, 
                          overwrite=True)
        self.assertFileExists(self.float_)
                          
    def test_simple_geotif_int(self):
        self.assertModule("t.rast.export", input="A", output=self.int_, 
                          overwrite=True, type="Int16")
        self.assertFileExists(self.int_)
                          
    def test_simple_grid(self):
        self.assertModule("t.rast.export", input="A", output=self.grid, 
                          overwrite=True, format="AAIGrid")
        self.assertFileExists(self.grid)
                          
    def test_simple_pack(self):
        self.assertModule("t.rast.export", input="A", output=self.pack, 
                          overwrite=True, format="pack")
        self.assertFileExists(self.pack)

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
