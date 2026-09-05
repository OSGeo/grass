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
from grass.lib.gis import G_remove_misc
from grass.lib.raster import (
    Rast_get_semantic_label_or_name,
    Rast_legal_semantic_label,
    Rast_read_semantic_label,
    Rast_write_semantic_label,
)
from grass.pygrass import utils
from grass.pygrass.gis import Mapset
from grass.script.core import tempname


class RastLegalBandIdTestCase(TestCase):
    def test_empty_name(self):
        ret = Rast_legal_semantic_label("")
        self.assertEqual(ret, False)
        ret = Rast_legal_semantic_label(" ")
        self.assertEqual(ret, False)

    def test_illegal_name(self):
        ret = Rast_legal_semantic_label(".a")
        self.assertEqual(ret, False)
        ret = Rast_legal_semantic_label("a/b")
        self.assertEqual(ret, False)
        ret = Rast_legal_semantic_label("a@b")
        self.assertEqual(ret, False)
        ret = Rast_legal_semantic_label("a#b")
        self.assertEqual(ret, False)

    def test_too_long(self):
        ret = Rast_legal_semantic_label(
            "a_" + "".join(random.choices(string.ascii_letters, k=253))
        )
        self.assertEqual(ret, True)
        ret = Rast_legal_semantic_label(
            "a_" + "".join(random.choices(string.ascii_letters, k=254))
        )
        self.assertEqual(ret, False)

    def test_good_name(self):
        ret = Rast_legal_semantic_label("1")
        self.assertEqual(ret, True)
        ret = Rast_legal_semantic_label("1a")
        self.assertEqual(ret, True)
        ret = Rast_legal_semantic_label("clouds")
        self.assertEqual(ret, True)
        ret = Rast_legal_semantic_label("rededge1")
        self.assertEqual(ret, True)
        ret = Rast_legal_semantic_label("S2_1")
        self.assertEqual(ret, True)
        ret = Rast_legal_semantic_label("GRASS_aspect_deg")
        self.assertEqual(ret, True)


class Rastsemantic_labelerenceTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.mapset = Mapset().name
        cls.semantic_label = "The_Doors"
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

    def test_read_semantic_label_present(self):
        Rast_write_semantic_label(self.map, self.semantic_label)
        ret = utils.decode(Rast_read_semantic_label(self.map, self.mapset))
        self.assertEqual(ret, self.semantic_label)

    def test_read_semantic_label_absent(self):
        G_remove_misc("cell_misc", "semantic_label", self.map)
        ret = Rast_read_semantic_label(self.map, self.mapset)
        self.assertFalse(bool(ret))

    def test_write_semantic_label(self):
        G_remove_misc("cell_misc", "semantic_label", self.map)
        Rast_write_semantic_label(self.map, self.semantic_label)
        ret = utils.decode(Rast_read_semantic_label(self.map, self.mapset))
        self.assertEqual(ret, self.semantic_label)

    def test_get_semantic_label_or_name(self):
        G_remove_misc("cell_misc", "semantic_label", self.map)
        ret = utils.decode(Rast_get_semantic_label_or_name(self.map, self.mapset))
        self.assertEqual(ret, self.map)
        Rast_write_semantic_label(self.map, self.semantic_label)
        ret = utils.decode(Rast_get_semantic_label_or_name(self.map, self.mapset))
        self.assertEqual(ret, self.semantic_label)


if __name__ == "__main__":
    test()
