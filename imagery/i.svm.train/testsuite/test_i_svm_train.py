"""
Name:      i.svm.train input & output tests
Purpose:   Validates user input validation code and output generation

Author:    Maris Nartiss
Copyright: (C) 2021 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""
import os
import unittest
import ctypes

from grass.script import core as grass
from grass.script import shutil_which
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.pygrass.gis import Mapset
from grass.pygrass import utils

from grass.lib.gis import (
    GPATH_MAX,
    GNAME_MAX,
    G_file_name_misc,
)
from grass.lib.imagery import (
    I_SIGFILE_TYPE_LIBSVM,
    I_get_signatures_dir,
    I_signatures_remove,
)


class IOValidationTest(TestCase):
    """Test input validation and output generation with i.svm.train"""

    @classmethod
    @unittest.skipIf(shutil_which("i.svm.train") is None, "i.svm.train not found.")
    def setUpClass(cls):
        cls.tmp_rasts = []
        cls.tmp_groups = []
        cls.tmp_sigs = []
        cls.mapset_name = Mapset().name
        # Small region for small testing rasters
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)
        # A raster without a semantic label
        cls.rast1 = grass.tempname(10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.rast1}=rand(0.0,1)", seed=1, quiet=True
        )
        cls.tmp_rasts.append(cls.rast1)
        cls.runModule("r.colors", _map=cls.rast1, color="grey", quiet=True)
        # A raster with a semantic label
        cls.rast2 = grass.tempname(10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.rast2}=rand(0.0,1)", seed=1, quiet=True
        )
        cls.tmp_rasts.append(cls.rast2)
        cls.runModule(
            "r.support", _map=cls.rast2, semantic_label="GRASS_RND1", quiet=True
        )
        cls.rast3 = grass.tempname(10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.rast3}=rand(0.0,1)", seed=1, quiet=True
        )
        cls.tmp_rasts.append(cls.rast3)
        cls.runModule(
            "r.support", _map=cls.rast3, semantic_label="GRASS_RND2", quiet=True
        )
        cls.rast4 = grass.tempname(10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.rast4}=rand(0.0,1)", seed=1, quiet=True
        )
        cls.tmp_rasts.append(cls.rast4)
        cls.runModule(
            "r.support", _map=cls.rast4, semantic_label="GRASS_RND3", quiet=True
        )
        # An empty imagery group
        cls.group1 = grass.tempname(10)
        cls.runModule("i.group", group=cls.group1, _input=(cls.rast1,), quiet=True)
        cls.tmp_groups.append(cls.group1)
        cls.runModule(
            "i.group", flags="r", group=cls.group1, _input=(cls.rast1,), quiet=True
        )
        # A good imagery group
        cls.group3 = grass.tempname(10)
        cls.runModule(
            "i.group", group=cls.group3, _input=(cls.rast2, cls.rast3), quiet=True
        )
        cls.tmp_groups.append(cls.group3)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.del_temp_region()
        for rast in cls.tmp_rasts:
            cls.runModule("g.remove", flags="f", _type="raster", name=rast)
        for group in cls.tmp_groups:
            cls.runModule("g.remove", flags="f", _type="group", name=group)
        for sig in cls.tmp_sigs:
            I_signatures_remove(I_SIGFILE_TYPE_LIBSVM, sig)

    @unittest.skipIf(shutil_which("i.svm.train") is None, "i.svm.train not found.")
    def test_empty_group(self):
        """Empty imagery group handling"""
        sigfile = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group1,
            _input=self.rast1,
            signaturefile=sigfile,
            quiet=True,
        )
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn(self.group1, isvm.outputs.stderr)

    @unittest.skipIf(shutil_which("i.svm.train") is None, "i.svm.train not found.")
    def test_wrong_sigfile_mapset(self):
        """Attempt to use FQ signature file name with not current mapset"""
        sigfile = grass.tempname(10)
        mapset = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            _input=self.rast1,
            signaturefile=f"{sigfile}@{mapset}",
            quiet=True,
        )
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn(mapset, isvm.outputs.stderr)

    @unittest.skipIf(shutil_which("i.svm.train") is None, "i.svm.train not found.")
    def test_wrong_svm_param(self):
        """Attempt to use invalid SVM parametres"""
        sigfile = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            _input=self.rast1,
            signaturefile=sigfile,
            eps=-1,
            quiet=True,
        )
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn("eps", isvm.outputs.stderr)

    @unittest.skipIf(shutil_which("i.svm.train") is None, "i.svm.train not found.")
    def test_creation_of_misc_files(self):
        """Validate creation of category, history and colour files"""
        sigfile = grass.tempname(10)
        csigdir = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(csigdir, I_SIGFILE_TYPE_LIBSVM)
        sigdir = utils.decode(csigdir.value)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            _input=self.rast1,
            signaturefile=sigfile,
            quiet=True,
        )
        self.assertModule(isvm)
        self.tmp_sigs.append(sigfile)
        cpath = ctypes.create_string_buffer(GPATH_MAX)
        G_file_name_misc(cpath, sigdir, "version", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "sig", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "cats", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "colr", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "history", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))

    @unittest.skipIf(shutil_which("i.svm.train") is None, "i.svm.train not found.")
    def test_dont_fail_if_misc_files_missing(self):
        """Colour file is missing but it should not cause a failure"""
        sigfile = grass.tempname(10)
        csigdir = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(csigdir, I_SIGFILE_TYPE_LIBSVM)
        sigdir = utils.decode(csigdir.value)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            _input=self.rast4,
            signaturefile=sigfile,
            quiet=True,
        )
        self.assertModule(isvm)
        self.tmp_sigs.append(sigfile)
        cpath = ctypes.create_string_buffer(GPATH_MAX)
        G_file_name_misc(cpath, sigdir, "version", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "sig", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "cats", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "colr", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertFalse(os.path.isfile(misc_file))
        G_file_name_misc(cpath, sigdir, "history", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))


if __name__ == "__main__":
    test()
