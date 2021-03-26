"""Test of raster library band name reference handling

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
import ctypes
import random
import string

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass.gis import Mapset
from grass.pygrass import utils

from grass.lib.raster import (
    Rast_legal_band_id,
    Rast_has_band_reference,
    Rast_write_band_reference,
    Rast_remove_band_reference,
    Rast_read_band_reference,
    Rast_find_band_filename,
)


class RastLegalBandIdTestCase(TestCase):
    def test_empty_name(self):
        ret = Rast_legal_band_id("")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id(" ")
        self.assertEqual(ret, -1)

    def test_illegal_name(self):
        ret = Rast_legal_band_id(".a")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("1")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("1a")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("a/b")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("a@b")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("a#b")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("GRASS")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("USER")
        self.assertEqual(ret, -1)

    def test_no_second_token(self):
        ret = Rast_legal_band_id("GRASS_")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("USER_")
        self.assertEqual(ret, -1)
        ret = Rast_legal_band_id("S2_")
        self.assertEqual(ret, -1)

    def test_too_long(self):
        ret = Rast_legal_band_id("".join(random.choices(string.ascii_letters, k=256)))
        self.assertEqual(ret, -1)

    def test_good_name(self):
        ret = Rast_legal_band_id("S2_1")
        self.assertEqual(ret, 1)
        ret = Rast_legal_band_id("GRASS_aspect_deg")
        self.assertEqual(ret, 1)


class RastBandReferenceTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.mapset = Mapset().name
        cls.band_filename = "{}.json".format(tempname(10))
        cls.band_id = "The_Doors"
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

    def test_bandref_missing(self):
        Rast_remove_band_reference(self.map)
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 0)

    def test_bandref_present(self):
        ret = Rast_write_band_reference(self.map, self.band_filename, self.band_id)
        self.assertEqual(ret, 1)
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 1)

    def test_bandref_remove(self):
        ret = Rast_write_band_reference(self.map, self.band_filename, self.band_id)
        self.assertEqual(ret, 1)
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 1)
        ret = Rast_remove_band_reference(self.map)
        self.assertEqual(ret, 1)
        ret = Rast_remove_band_reference(self.map)
        self.assertEqual(ret, 0)
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 0)

    def test_bandref_read(self):
        ret = Rast_write_band_reference(self.map, self.band_filename, self.band_id)
        self.assertEqual(ret, 1)
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 1)
        p_filename = ctypes.pointer(ctypes.create_string_buffer(4096))
        p_band_id = ctypes.pointer(ctypes.create_string_buffer(4096))
        ret = Rast_read_band_reference(
            self.map, self.mapset, ctypes.pointer(p_filename), ctypes.pointer(p_band_id)
        )
        self.assertEqual(ret, 1)
        band_id = utils.decode(p_band_id.contents.value)
        band_file = utils.decode(p_filename.contents.value)
        self.assertEqual(band_id, self.band_id)
        self.assertEqual(band_file, self.band_filename)


class RastBandFindFilenameTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.doesnt_exist = tempname(10)
        # Create a user defined band metadata file
        # (touch $HOME/.grass8/band_meta/RANDOMFILE.json)
        # Should be done via C user band management function

    @classmethod
    def tearDownClass(cls):
        # Remove user defined band metadata file created in setUpClass
        pass

    def test_unknown_filename(self):
        ret = Rast_find_band_filename(self.doesnt_exist)
        self.assertFalse(ret)

    def test_global_filename(self):
        # TODO: needs a global band metadata file
        pass

    def test_user_filename(self):
        # TODO: needs user band management C functions
        pass


if __name__ == "__main__":
    test()
