"""Test of raster library metadata handling

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main

=======
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
import random
import string

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass.gis import Mapset
from grass.pygrass import utils

from grass.lib.gis import G_remove_misc
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
from grass.lib.raster import (
    Rast_legal_semantic_label,
    Rast_read_semantic_label,
    Rast_get_semantic_label_or_name,
    Rast_write_semantic_label,
)
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
from grass.lib.raster import Rast_legal_bandref, Rast_read_bandref, Rast_write_bandref
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main


class RastLegalBandIdTestCase(TestCase):
    def test_empty_name(self):
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        ret = Rast_legal_semantic_label("")
        self.assertEqual(ret, False)
        ret = Rast_legal_semantic_label(" ")
        self.assertEqual(ret, False)
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main

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
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        ret = Rast_legal_bandref("")
        self.assertEqual(ret, -1)
        ret = Rast_legal_bandref(" ")
        self.assertEqual(ret, -1)
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

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


<<<<<<< HEAD
class RastBandReferenceTestCase(TestCase):
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
class Rastsemantic_labelerenceTestCase(TestCase):
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.mapset = Mapset().name
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        cls.semantic_label = "The_Doors"
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        cls.semantic_label = "The_Doors"
=======
=======
<<<<<<< HEAD
<<<<<<< HEAD
        cls.semantic_label = "The_Doors"
=======
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
        cls.semantic_label = "The_Doors"
=======
>>>>>>> osgeo-main
        cls.bandref = "The_Doors"
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
        cls.semantic_label = "The_Doors"
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        cls.semantic_label = "The_Doors"
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> osgeo-main
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    def test_read_semantic_label_present(self):
        Rast_write_semantic_label(self.map, self.semantic_label)
        ret = utils.decode(Rast_read_semantic_label(self.map, self.mapset))
        self.assertEqual(ret, self.semantic_label)
<<<<<<< HEAD
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
=======
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
>>>>>>> osgeo-main

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
<<<<<<< HEAD
=======
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
    def test_read_bandref_present(self):
        Rast_write_bandref(self.map, self.bandref)
        ret = utils.decode(Rast_read_bandref(self.map, self.mapset))
        self.assertEqual(ret, self.bandref)
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

    def test_read_semantic_label_absent(self):
        G_remove_misc("cell_misc", "semantic_label", self.map)
        ret = Rast_read_semantic_label(self.map, self.mapset)
        self.assertFalse(bool(ret))

<<<<<<< HEAD
    def test_write_bandref(self):
        G_remove_misc("cell_misc", "bandref", self.map)
        Rast_write_bandref(self.map, self.bandref)
        ret = utils.decode(Rast_read_bandref(self.map, self.mapset))
        self.assertEqual(ret, self.bandref)
>>>>>>> da7f79c3f9 (libpython: Save and load benchmark results (#1711))
=======
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
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main
=======
>>>>>>> osgeo-main


if __name__ == "__main__":
    test()
