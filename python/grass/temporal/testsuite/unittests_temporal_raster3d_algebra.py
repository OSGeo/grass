"""
(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""


import grass.script
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import datetime


class TestTemporalRaster3dAlgebra(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        ret = grass.script.run_command("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=100.0, b=0.0, res=10.0)

        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")

        tgis.open_new_stds(name="A", type="str3ds", temporaltype="absolute",
                           title="A", descr="A", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster_3d", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)

    def tearDown(self):
        self.runModule("t.remove", type="str3ds", flags="rf", inputs="D", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        #cls.runModule("t.remove", type="str3ds", flags="rf", inputs="A", quiet=True)
        cls.del_temp_region()

    def test_temporal_neighbors_1(self):
        """Simple temporal neighborhood computation test"""
        A = tgis.open_old_stds("A", type="str3ds")
        A.print_info()

        tra = tgis.TemporalRaster3DAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A[-1] + A[1]',
                  basename="d", overwrite=True)

        D = tgis.open_old_stds("D", type="str3ds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

    def test_temporal_neighbors_2(self):
        """Simple temporal neighborhood computation test"""
        A = tgis.open_old_stds("A", type="str3ds")
        A.print_info()

        tra = tgis.TemporalRaster3DAlgebraParser(run = True, debug = True)
        tra.parse(expression='D = A[0,0,0,-1] + A[0,0,0,1]',
                  basename="d", overwrite=True)

        D = tgis.open_old_stds("D", type="str3ds")
        self.assertTrue(D.is_in_db())
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)  # 1 + 3
        self.assertEqual(D.metadata.get_max_max(), 6) # 2 + 4
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))


if __name__ == '__main__':
    test()
