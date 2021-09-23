"""Test of raster library metadata handling

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
import random
import string

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass.gis import Mapset
from grass.pygrass import utils

from grass.lib.gis import G_remove_misc
from grass.lib.raster import (
    Rast_legal_bandref,
    Rast_read_bandref,
    Rast_get_bandref_or_name,
    Rast_write_bandref,
)


class RastLegalBandIdTestCase(TestCase):
    def test_empty_name(self):
        ret = Rast_legal_bandref("")
        self.assertEqual(ret, -1)
        ret = Rast_legal_bandref(" ")
        self.assertEqual(ret, -1)

    def test_illegal_name(self):
        ret = Rast_legal_bandref(".a")
        self.assertEqual(ret, -1)
        ret = Rast_legal_bandref("a/b")
        self.assertEqual(ret, -1)
        ret = Rast_legal_bandref("a@b")
        self.assertEqual(ret, -1)
        ret = Rast_legal_bandref("a#b")
        self.assertEqual(ret, -1)

    def test_too_long(self):
        ret = Rast_legal_bandref(
            "a_" + "".join(random.choices(string.ascii_letters, k=253))
        )
        self.assertEqual(ret, 1)
        ret = Rast_legal_bandref(
            "a_" + "".join(random.choices(string.ascii_letters, k=254))
        )
        self.assertEqual(ret, -1)

    def test_good_name(self):
        ret = Rast_legal_bandref("1")
        self.assertEqual(ret, 1)
        ret = Rast_legal_bandref("1a")
        self.assertEqual(ret, 1)
        ret = Rast_legal_bandref("clouds")
        self.assertEqual(ret, 1)
        ret = Rast_legal_bandref("rededge1")
        self.assertEqual(ret, 1)
        ret = Rast_legal_bandref("S2_1")
        self.assertEqual(ret, 1)
        ret = Rast_legal_bandref("GRASS_aspect_deg")
        self.assertEqual(ret, 1)


class RastBandReferenceTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.mapset = Mapset().name
        cls.bandref = "The_Doors"
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

    def test_read_bandref_present(self):
        Rast_write_bandref(self.map, self.bandref)
        ret = utils.decode(Rast_read_bandref(self.map, self.mapset))
        self.assertEqual(ret, self.bandref)

    def test_read_bandref_absent(self):
        G_remove_misc("cell_misc", "bandref", self.map)
        ret = Rast_read_bandref(self.map, self.mapset)
        self.assertFalse(bool(ret))

    def test_write_bandref(self):
        G_remove_misc("cell_misc", "bandref", self.map)
        Rast_write_bandref(self.map, self.bandref)
        ret = utils.decode(Rast_read_bandref(self.map, self.mapset))
        self.assertEqual(ret, self.bandref)

    def test_get_bandref_or_name(self):
        G_remove_misc("cell_misc", "bandref", self.map)
        ret = utils.decode(Rast_get_bandref_or_name(self.map, self.mapset))
        self.assertEqual(ret, self.map)
        Rast_write_bandref(self.map, self.bandref)
        ret = utils.decode(Rast_get_bandref_or_name(self.map, self.mapset))
        self.assertEqual(ret, self.bandref)


if __name__ == "__main__":
    test()
