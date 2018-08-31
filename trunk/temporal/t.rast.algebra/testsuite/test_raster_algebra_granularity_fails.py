"""
(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""

import os
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestTRastAlgebraGranularityFails(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE",  "1")
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a1 = 7")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a2 = 8")


        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="A",  maps="a1", 
                                                start="2001-02-01", end="2001-04-01")
        tgis.register_maps_in_space_time_dataset(type="raster", name="A",  maps="a2", 
                                                start="2001-03-01", end="2001-05-01")
        
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region 
        """
        cls.runModule("t.remove", flags="rf", inputs="A", quiet=True)
        cls.runModule("t.unregister", maps="singletmap", quiet=True)
        cls.del_temp_region()

    def test_error_handling(self):        
        # Syntax error
        self.assertModuleFail("t.rast.algebra", flags="g",  expression="R = A {+,equal| precedes| follows,l A", basename="r")     
        # Syntax error
        self.assertModuleFail("t.rast.algebra", flags="g",  expression="R = A {+,equal| precedes| follows,l} A", basename="r")
        # Syntax error
        self.assertModuleFail("t.rast.algebra", flags="g",  expression="R == A + A", basename="r")
        # No STRDS
        self.assertModuleFail("t.rast.algebra", flags="g",  expression="R = NoSTRDS + NoSTRDS", basename="r")
        # No basename
        self.assertModuleFail("t.rast.algebra", flags="g",  expression="R = A + A")
        # Invalid temporal topology
        self.assertModuleFail("t.rast.algebra", flags="g",  expression="R = A + A", basename="r")


if __name__ == '__main__':
    test()
