"""!Unit test to register raster maps with absolute and relative 
   time using tgis.register_maps_in_space_time_dataset()

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import grass.script as grass
import grass.temporal as tgis
import unittest
import datetime

class TestRegisterFunctions(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """!Initiate the temporal GIS and set the region
        """
        tgis.init(True)
        grass.overwrite = True
        grass.use_temp_region()
        ret = grass.run_command("g.region", n=80.0, s=0.0, e=120.0, 
                                w=0.0, t=1.0, b=0.0, res=10.0)

    def setUp(self):
        """!Create the test maps
        """
        ret = 0
        ret += grass.run_command("r.mapcalc", overwrite=True, quiet=True, 
                          expression="register_map_1 = 1")
        ret += grass.run_command("r.mapcalc", overwrite=True, quiet=True, 
                          expression="register_map_2 = 2")
        self.assertEqual(ret, 0)

    def test_absolute_time_strds(self):
        """!Test the registration of maps with absolute time in a
           space time raster dataset
        """
        ret = grass.run_command("t.create", output="register_test", 
                                title="Test strds", description="Test strds",
                                temporaltype="absolute")
        self.assertEqual(ret, 0)

        tgis.register_maps_in_space_time_dataset(type="rast", name="register_test", 
                 maps="register_map_1,register_map_2",
                 start="2001-01-01", increment="1 day", interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, tz = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, tz = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        strds = tgis.SpaceTimeRasterDataset("register_test@" + tgis.get_current_mapset())
        strds.select()
        start, end, tz = strds.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        ret = grass.run_command("t.remove", input="register_test") 
        self.assertEqual(ret, 0)

    def test_absolute_time_1(self):
        """!Test the registration of maps with absolute time
        """
        tgis.register_maps_in_space_time_dataset(type="rast", name=None, 
                 maps="register_map_1,register_map_2",
                 start="2001-01-01", increment="1 day", interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, tz = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, tz = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

    def test_absolute_time_2(self):
        """!Test the registration of maps with absolute time
        """
        tgis.register_maps_in_space_time_dataset(type="rast", name=None, 
                 maps="register_map_1,register_map_2",
                 start="2001-01-01 10:30:01", increment="8 hours", interval=False)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, tz = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1, 10, 30, 1))

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, tz = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1, 18, 30, 1))

    def test_relative_time_strds(self):
        """!Test the registration of maps with relative time in a
           space time raster dataset
        """
        ret = grass.run_command("t.create", output="register_test", 
                                title="Test strds", description="Test strds",
                                temporaltype="relative")
        self.assertEqual(ret, 0)


        tgis.register_maps_in_space_time_dataset(type="rast", name="register_test", 
                 maps="register_map_1,register_map_2",
                 start=0, increment=1, unit="day", interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 0)
        self.assertEqual(end, 1)
        self.assertEqual(unit, "day")

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1)
        self.assertEqual(end, 2)
        self.assertEqual(unit, "day")

        strds = tgis.SpaceTimeRasterDataset("register_test@" + tgis.get_current_mapset())
        strds.select()
        start, end, unit = strds.get_relative_time()
        self.assertEqual(start, 0)
        self.assertEqual(end, 2)
        self.assertEqual(unit, "day")

        ret = grass.run_command("t.remove", input="register_test") 
        self.assertEqual(ret, 0)

    def test_relative_time_1(self):
        """!Test the registration of maps with relative time
        """
        tgis.register_maps_in_space_time_dataset(type="rast", name=None, 
                 maps="register_map_1,register_map_2",
                 start=0, increment=1, unit="day", interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 0)
        self.assertEqual(end, 1)
        self.assertEqual(unit, "day")

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1)
        self.assertEqual(end, 2)
        self.assertEqual(unit, "day")

    def test_relative_time_2(self):
        """!Test the registration of maps with relative time
        """
        tgis.register_maps_in_space_time_dataset(type="rast", name=None, 
                 maps="register_map_1,register_map_2",
                 start=1000000, increment=500000, unit="second", interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1000000)
        self.assertEqual(end, 1500000)
        self.assertEqual(unit, "second")

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1500000)
        self.assertEqual(end, 2000000)
        self.assertEqual(unit, "second")

    def tearDown(self):
        """!Remove maps from temporal database
        """
        ret = grass.run_command("t.unregister", maps="register_map_1,register_map_2")
        self.assertEqual(ret, 0)

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        grass.del_temp_region()

if __name__ == '__main__':
    unittest.main()


