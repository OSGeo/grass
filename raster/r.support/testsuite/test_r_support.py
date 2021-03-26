"""Test of r.support basic functionality

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
    Rast_has_band_reference,
    Rast_remove_band_reference,
    Rast_read_band_reference,
)


class RSupportBandHandlingTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.mapset = Mapset().name
        cls.unknown_band_filename = tempname(10)
        cls.band_filename = "{}.json".format(tempname(10))
        cls.band_id = "The_Doors"
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

    def test_exclusitivity(self):
        self.assertModuleFail(
            "r.support",
            map=self.map,
            band_id=self.band_id,
            b=True
        )

    def test_band_id_invalid(self):
        self.assertModuleFail(
            "r.support",
            map=self.map,
            band_id="".join(random.choices(string.ascii_letters, k=256))
        )

    def test_filename_too_long(self):
        self.assertModuleFail(
            "r.support",
            map=self.map,
            band_filename="".join(random.choices(string.ascii_letters, k=256))
        )

    def test_unknown_filename(self):
        self.assertModuleFail(
            "r.support",
            map=self.map,
            band_filename=self.unknown_band_filename
        )

    def test_set_band_id(self):
        Rast_remove_band_reference(self.map)
        self.assertModule(
            "r.support",
            map=self.map,
            band_id=self.band_id
        )
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 1)
        p_filename = ctypes.pointer(ctypes.create_string_buffer(4096))
        p_band_id = ctypes.pointer(ctypes.create_string_buffer(4096))
        ret = Rast_read_band_reference(
            self.map, self.mapset, ctypes.pointer(p_filename), ctypes.pointer(p_band_id)
        )
        self.assertEqual(ret, 1)
        band_id = utils.decode(p_band_id.contents.value)
        self.assertEqual(band_id, self.band_id)

    def test_remove_band_ref(self):
        self.assertModule(
            "r.support",
            map=self.map,
            band_id=self.band_id
        )
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 1)
        self.assertModule(
            "r.support",
            map=self.map,
            b=True
        )
        ret = Rast_has_band_reference(self.map, self.mapset)
        self.assertEqual(ret, 0)


if __name__ == "__main__":
    test()
