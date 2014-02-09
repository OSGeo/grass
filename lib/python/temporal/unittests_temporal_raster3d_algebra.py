"""!Unit test to register raster maps with absolute and relative
   time using tgis.register_maps_in_space_time_dataset()

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import grass.script
import grass.temporal as tgis
import unittest
import datetime
import os

class TestRegisterFunctions(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """!Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        grass.script.use_temp_region()
        ret = grass.script.run_command("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=100.0, b=0.0, res=10.0)

        ret += grass.script.run_command("r3.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        ret += grass.script.run_command("r3.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        ret += grass.script.run_command("r3.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        ret += grass.script.run_command("r3.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")


        tgis.open_new_space_time_dataset(name="A", type="str3ds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="rast3d", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)

    def test_temporal_neighbors_1(self):
        """Simple temporal neighborhood computation test"""
        tra = tgis.TemporalRaster3DAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A[-1] + A[1]',
                  basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_neighbors_2(self):
        """Simple temporal neighborhood computation test"""
        tra = tgis.TemporalRaster3DAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A[0,0,0,-1] + A[0,0,0,1]',
                  basename="d", overwrite=True)

        D = tgis.open_old_space_time_dataset("D", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def tearDown(self):
        ret = grass.script.run_command("t.remove", type="str3ds", flags="rf", input="D", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        ret = grass.script.run_command("t.remove", type="str3ds", flags="rf", input="A", quiet=True)
        grass.script.del_temp_region()

if __name__ == '__main__':
    unittest.main()


