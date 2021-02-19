"""Unit test to register raster maps with absolute and relative
   time using tgis.register_maps_in_space_time_dataset()

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""

import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import datetime
import os

class TestRasterRegisterFunctions(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE", "1")
        # Use always the current mapset as temporal database
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule('g.region', n=80.0, s=0.0, e=120.0, w=0.0,
                      t=1.0, b=0.0, res=10.0)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()

    def setUp(self):
        """Create the test maps and the space time raster datasets
        """
        self.runModule("r.mapcalc", overwrite=True, quiet=True,
                       expression="register_map_1 = 1")
        self.runModule("r.mapcalc", overwrite=True, quiet=True,
                       expression="register_map_2 = 2")
        self.runModule("r.mapcalc", overwrite=True, quiet=True,
                       expression="register_map_null = null()")

        self.strds_abs = tgis.open_new_stds(name="register_test_abs", type="strds", temporaltype="absolute",
                                            title="Test strds", descr="Test strds", semantic="field",
                                            overwrite=True)
        self.strds_rel = tgis.open_new_stds(name="register_test_rel", type="strds", temporaltype="relative",
                                            title="Test strds", descr="Test strds", semantic="field",
                                            overwrite=True)

    def tearDown(self):
        """Remove maps from temporal database
        """
        self.runModule("t.unregister", type="raster", maps="register_map_1,register_map_2",
                       quiet=True)
        self.runModule("g.remove", flags='f', type="raster", name="register_map_1,register_map_2",
                       quiet=True)
        self.strds_abs.delete()
        self.strds_rel.delete()

    def test_absolute_time_strds_1(self):
        """Test the registration of maps with absolute time in a
           space time raster dataset
        """
        tgis.register_maps_in_space_time_dataset(type="raster", name=self.strds_abs.get_name(),
                 maps="register_map_1,register_map_2",
                 start="2001-01-01", increment="1 day", interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        self.strds_abs.select()
        start, end = self.strds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

    def test_absolute_time_strds_2(self):
        """Test the registration of maps with absolute time in a
           space time raster dataset.
           The timestamps are set using the C-Interface beforehand, so that the register function needs
           to read the timetsamp from the map metadata.
        """

        ciface = tgis.get_tgis_c_library_interface()
        ciface.write_raster_timestamp("register_map_1", tgis.get_current_mapset(), "1 Jan 2001/2 Jan 2001")
        ciface.write_raster_timestamp("register_map_2", tgis.get_current_mapset(), "2 Jan 2001/3 Jan 2001")

        tgis.register_maps_in_space_time_dataset(type="raster", name=self.strds_abs.get_name(),
                                                 maps="register_map_1,register_map_2")

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        self.strds_abs.select()
        start, end = self.strds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

    def test_absolute_time_strds_3(self):
        """Test the registration of maps with absolute time in a
           space time raster dataset. The timestamps are set via method arguments and with the
           c-interface. The timestamps of the method arguments should overwrite the
           time stamps set via the C-interface.
        """

        ciface = tgis.get_tgis_c_library_interface()
        ciface.write_raster_timestamp("register_map_1", tgis.get_current_mapset(), "1 Jan 2001/2 Jan 2001")

        tgis.register_maps_in_space_time_dataset(type="raster", name=self.strds_abs.get_name(),
                                                 maps="register_map_1",
                                                 start="2001-02-01", increment="1 day",
                                                 interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 2, 2))

        self.strds_abs.select()
        start, end = self.strds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 2, 2))

    def test_absolute_time_strds_4(self):
        """Test the registration of maps with absolute time in a
           space time raster dataset. The timestamps are set via method arguments and with the
           c-interface. The timestamps of the method arguments should overwrite the
           time stamps set via the C-interface. The C-interface sets relative time stamps.
        """

        ciface = tgis.get_tgis_c_library_interface()
        # Set the timestamp as relative time
        ciface.write_raster_timestamp("register_map_1", tgis.get_current_mapset(), "1 day")

        tgis.register_maps_in_space_time_dataset(type="raster", name=self.strds_abs.get_name(),
                                                 maps="register_map_1",
                                                 start="2001-02-01", increment="1 day",
                                                 interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 2, 2))

        self.strds_abs.select()
        start, end = self.strds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 2, 2))

    def test_absolute_time_1(self):
        """Test the registration of maps with absolute time
        using register_maps_in_space_time_dataset() and register_map_object_list()
        """
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,
                 maps="register_map_1,register_map_2",
                 start="2001-01-01", increment="1 day", interval=True)

        map_1 = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map_1.select()
        start, end = map_1.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map_2 = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map_2.select()
        start, end = map_2.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        map_list = [map_1, map_2]

        tgis.register_map_object_list(type="raster", map_list=map_list,
                                      output_stds=self.strds_abs,
                                      delete_empty=False)
        self.strds_abs.select()
        start, end = self.strds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

    def test_absolute_time_2(self):
        """Test the registration of maps with absolute time
        using register_maps_in_space_time_dataset() and register_map_object_list() with empty map deletion
        """
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,
                 maps="register_map_1,register_map_2,register_map_null",
                 start="2001-01-01 10:30:01", increment="8 hours", interval=False)

        map_1 = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map_1.select()
        start, end = map_1.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1, 10, 30, 1))

        map_2 = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map_2.select()
        start, end = map_2.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1, 18, 30, 1))

        map_3 = tgis.RasterDataset("register_map_null@" + tgis.get_current_mapset())
        map_3.select()
        start, end = map_3.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2, 2, 30, 1))

        map_list = [map_1, map_2, map_3]

        tgis.register_map_object_list(type="raster", map_list=map_list,
                                      output_stds=self.strds_abs,
                                      delete_empty=True)
        self.strds_abs.select()
        start, end = self.strds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1, 10, 30, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 1, 18, 30, 1))

        map_3 = tgis.VectorDataset("register_map_null@" + tgis.get_current_mapset())
        self.assertEqual(map_3.map_exists(), False)

    def test_absolute_time_3(self):
        """Test the registration of maps with absolute time.
           The timestamps are set using the C-Interface beforehand, so that the register function needs
           to read the timetsamp from the map metadata.
        """

        ciface = tgis.get_tgis_c_library_interface()
        ciface.write_raster_timestamp("register_map_1", tgis.get_current_mapset(), "1 Jan 2001 10:30:01")
        ciface.write_raster_timestamp("register_map_2", tgis.get_current_mapset(), "1 Jan 2001 18:30:01")

        tgis.register_maps_in_space_time_dataset(type="raster", name=None,
                 maps="register_map_1,register_map_2")

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1, 10, 30, 1))

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1, 18, 30, 1))

    def test_relative_time_strds_1(self):
        """Test the registration of maps with relative time in a
           space time raster dataset
        """

        tgis.register_maps_in_space_time_dataset(type="raster", name=self.strds_rel.get_name(),
                                                 maps="register_map_1,register_map_2", start=0,
                                                 increment=1, unit="day", interval=True)

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

        self.strds_rel.select()
        start, end, unit = self.strds_rel.get_relative_time()
        self.assertEqual(start, 0)
        self.assertEqual(end, 2)
        self.assertEqual(unit, "day")

    def test_relative_time_strds_2(self):
        """Test the registration of maps with relative time in a
           space time raster dataset. The timetsamps are set for the maps using the
           C-interface before registration.
        """
        ciface = tgis.get_tgis_c_library_interface()
        ciface.write_raster_timestamp("register_map_1", tgis.get_current_mapset(), "1000000 seconds/1500000 seconds")
        ciface.write_raster_timestamp("register_map_2", tgis.get_current_mapset(), "1500000 seconds/2000000 seconds")

        tgis.register_maps_in_space_time_dataset(type="raster", name=self.strds_rel.get_name(),
                                                 maps="register_map_1,register_map_2")

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1000000)
        self.assertEqual(end, 1500000)
        self.assertEqual(unit, "seconds")

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1500000)
        self.assertEqual(end, 2000000)
        self.assertEqual(unit, "seconds")

        self.strds_rel.select()
        start, end, unit = self.strds_rel.get_relative_time()
        self.assertEqual(start, 1000000)
        self.assertEqual(end, 2000000)
        self.assertEqual(unit, "seconds")

    def test_relative_time_1(self):
        """Test the registration of maps with relative time
        """
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,
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
        """Test the registration of maps with relative time
        """
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,
                 maps="register_map_1,register_map_2",
                 start=1000000, increment=500000, unit="seconds", interval=True)

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1000000)
        self.assertEqual(end, 1500000)
        self.assertEqual(unit, "seconds")

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1500000)
        self.assertEqual(end, 2000000)
        self.assertEqual(unit, "seconds")

    def test_relative_time_3(self):
        """Test the registration of maps with relative time. The timetsamps are set beforehand using
           the C-interface.
        """
        ciface = tgis.get_tgis_c_library_interface()
        ciface.write_raster_timestamp("register_map_1", tgis.get_current_mapset(), "1000000 seconds/1500000 seconds")
        ciface.write_raster_timestamp("register_map_2", tgis.get_current_mapset(), "1500000 seconds/2000000 seconds")

        tgis.register_maps_in_space_time_dataset(type="raster", name=None,
                 maps="register_map_1,register_map_2")

        map = tgis.RasterDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1000000)
        self.assertEqual(end, 1500000)
        self.assertEqual(unit, "seconds")

        map = tgis.RasterDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end, unit = map.get_relative_time()
        self.assertEqual(start, 1500000)
        self.assertEqual(end, 2000000)
        self.assertEqual(unit, "seconds")


class TestVectorRegisterFunctions(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        os.putenv("GRASS_OVERWRITE", "1")
        # Use always the current mapset as temporal database
        cls.runModule("g.gisenv", set="TGIS_USE_CURRENT_MAPSET=1")
        tgis.init()
        cls.use_temp_region()
        cls.runModule('g.region', n=80.0, s=0.0, e=120.0, w=0.0,
                      t=1.0, b=0.0, res=10.0)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()

    def setUp(self):
        """Create the test maps and the space time raster datasets
        """
        self.runModule("v.random", overwrite=True, quiet=True,
                       output="register_map_1", npoints=5, seed=1)
        self.runModule("v.random", overwrite=True, quiet=True,
                       output="register_map_2", npoints=5, seed=1)
        self.runModule("r.mapcalc", overwrite=True, quiet=True,
                       expression="register_map_null = null()")
        self.runModule("r.to.vect", overwrite=True, quiet=True,
                       input="register_map_null", type="point",
                       output="register_map_empty")

        self.stvds_abs = tgis.open_new_stds(name="register_test_abs", type="stvds", temporaltype="absolute",
                                            title="Test stvds", descr="Test stvds", semantic="field",
                                            overwrite=True)
        self.stvds_rel = tgis.open_new_stds(name="register_test_rel", type="stvds", temporaltype="relative",
                                            title="Test stvds", descr="Test stvds", semantic="field",
                                            overwrite=True)

    def tearDown(self):
        """Remove maps from temporal database
        """
        self.runModule("t.unregister", type="vector", maps="register_map_1,register_map_2", quiet=True)
        self.runModule("g.remove", flags='f', type="vector", name="register_map_1,register_map_2", quiet=True)
        self.stvds_abs.delete()
        self.stvds_rel.delete()

    def test_absolute_time_stvds_1(self):
        """Test the registration of maps with absolute time in a
           space time raster dataset
        """
        tgis.register_maps_in_space_time_dataset(type="vector", name=self.stvds_abs.get_name(),
                 maps="register_map_1,register_map_2",
                 start="2001-01-01", increment="1 day", interval=True)

        map = tgis.VectorDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map = tgis.VectorDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        self.stvds_abs.select()
        start, end = self.stvds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

    def test_absolute_time_stvds_2(self):
        """Test the registration of maps with absolute time in a
           space time raster dataset.
           The timestamps are set using the C-Interface beforehand, so that the register function needs
           to read the timetsamp from the map metadata.
        """

        ciface = tgis.get_tgis_c_library_interface()
        ciface.write_vector_timestamp("register_map_1", tgis.get_current_mapset(), "1 Jan 2001/2 Jan 2001")
        ciface.write_vector_timestamp("register_map_2", tgis.get_current_mapset(), "2 Jan 2001/3 Jan 2001")

        tgis.register_maps_in_space_time_dataset(type="vector", name=self.stvds_abs.get_name(),
                                                 maps="register_map_1,register_map_2")

        map = tgis.VectorDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map = tgis.VectorDataset("register_map_2@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        self.stvds_abs.select()
        start, end = self.stvds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

    def test_absolute_time_stvds_3(self):
        """Test the registration of maps with absolute time in a
           space time raster dataset. The timestamps are set via method arguments and with the
           c-interface. The timestamps of the method arguments should overwrite the
           time stamps set via the C-interface.
        """

        ciface = tgis.get_tgis_c_library_interface()
        ciface.write_vector_timestamp("register_map_1", tgis.get_current_mapset(), "1 Jan 2001/2 Jan 2001")

        tgis.register_maps_in_space_time_dataset(type="vector", name=self.stvds_abs.get_name(),
                                                 maps="register_map_1",
                                                 start="2001-02-01", increment="1 day",
                                                 interval=True)

        map = tgis.VectorDataset("register_map_1@" + tgis.get_current_mapset())
        map.select()
        start, end = map.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 2, 2))

        self.stvds_abs.select()
        start, end = self.stvds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 2, 1))
        self.assertEqual(end, datetime.datetime(2001, 2, 2))

    def test_absolute_time_1(self):
        """Register vector maps in the temporal database and in addition in a stvds using the object method

        :return:
        """
        tgis.register_maps_in_space_time_dataset(type="vector", name=None,
                 maps="register_map_1,register_map_2",
                 start="2001-01-01", increment="1 day", interval=True)

        map_1 = tgis.VectorDataset("register_map_1@" + tgis.get_current_mapset())
        map_1.select()
        start, end = map_1.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map_2 = tgis.VectorDataset("register_map_2@" + tgis.get_current_mapset())
        map_2.select()
        start, end = map_2.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        map_list = [map_1, map_2]

        tgis.register_map_object_list(type="vector", map_list=map_list,
                                      output_stds=self.stvds_abs,
                                      delete_empty=False)
        self.stvds_abs.select()
        start, end = self.stvds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

    def test_absolute_time_2(self):
        """Register vector maps in the temporal database and in addition
        in a stvds using the object method deleting empty maps

        :return:
        """
        tgis.register_maps_in_space_time_dataset(type="vector", name=None,
                 maps="register_map_1,register_map_2,register_map_empty",
                 start="2001-01-01", increment="1 day", interval=True)

        map_1 = tgis.VectorDataset("register_map_1@" + tgis.get_current_mapset())
        map_1.select()
        start, end = map_1.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 2))

        map_2 = tgis.VectorDataset("register_map_2@" + tgis.get_current_mapset())
        map_2.select()
        start, end = map_2.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        map_3 = tgis.VectorDataset("register_map_empty@" + tgis.get_current_mapset())
        map_3.select()
        start, end = map_3.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 3))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))

        map_list = [map_1, map_2, map_3]

        tgis.register_map_object_list(type="vector", map_list=map_list,
                                      output_stds=self.stvds_abs,
                                      delete_empty=True)
        self.stvds_abs.select()
        start, end = self.stvds_abs.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 3))

        map_3 = tgis.VectorDataset("register_map_empty@" + tgis.get_current_mapset())
        self.assertEqual(map_3.map_exists(), False)


class TestRegisterFails(TestCase):

    def test_error_handling_1(self):
        # start option is missing
        self.assertModuleFail("t.register", input="test", end='2001-01-01', maps=("a", "b"))

    def test_error_handling_2(self):
        # No input definition
        self.assertModuleFail("t.register", input="test", start='2001-01-01')

    def test_error_handling_3(self):
        # File and maps are mutually exclusive
        self.assertModuleFail("t.register", input="test", start='2001-01-01', maps=("a", "b"), file="maps.txt")

    def test_error_handling_4(self):
        # Increment needs start
        self.assertModuleFail("t.register", input="test", increment="1 day", maps=("a", "b"))

    def test_error_handling_5(self):
        # Interval needs start
        self.assertModuleFail("t.register", flags="i", input="test", maps=("a", "b"))

    def test_error_handling_6(self):
        # Increment and end are mutually exclusive
        self.assertModuleFail("t.register", input="test", start='2001-01-01', end='2001-01-01',
                              increment="1 day", maps=("a", "b"))

    def test_error_handling_7(self):
        # Interval and end are mutually exclusive
        self.assertModuleFail("t.register", flags="i", input="test", start='2001-01-01', end='2001-01-01',
                              maps=("a", "b"))


if __name__ == '__main__':
    test()
