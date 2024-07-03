"""Test of r.support basic functionality

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
from grass.lib.raster import Rast_read_semantic_label


class RSupportSemanticLabelHandlingTestCase(TestCase):
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

    def test_exclusitivity(self):
        self.assertModuleFail(
            "r.support", map=self.map, semantic_label=self.semantic_label, b=True
        )

    def test_semantic_label_invalid(self):
        self.assertModuleFail(
            "r.support",
            map=self.map,
            semantic_label="".join(random.choices(string.ascii_letters, k=256)),
        )

    def test_set_semantic_label(self):
        G_remove_misc("cell_misc", "semantic_label", self.map)
        self.assertModule("r.support", map=self.map, semantic_label=self.semantic_label)
        ret = utils.decode(Rast_read_semantic_label(self.map, self.mapset))
        self.assertEqual(ret, self.semantic_label)

    def test_remove_semantic_label(self):
        self.assertModule("r.support", map=self.map, semantic_label=self.semantic_label)
        ret = Rast_read_semantic_label(self.map, self.mapset)
        self.assertTrue(bool(ret))
        self.assertModule("r.support", map=self.map, b=True)
        ret = Rast_read_semantic_label(self.map, self.mapset)
        self.assertFalse(bool(ret))


if __name__ == "__main__":
    test()
