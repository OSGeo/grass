"""
Name:      i.svm.train input & output tests
Purpose:   Validates user input validation code and output generation

Author:    Maris Nartiss
Copyright: (C) 2023 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import unittest
import ctypes
import shutil

from grass.script import core as grass
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
    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
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
        cls.rast5 = grass.tempname(10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.rast5}=rand(-1.0,1)", seed=1, quiet=True
        )
        cls.tmp_rasts.append(cls.rast5)
        cls.runModule(
            "r.support", _map=cls.rast5, semantic_label="GRASS_RND4", quiet=True
        )
        cls.rast6 = grass.tempname(10)
        cls.runModule(
            "r.mapcalc",
            expression=(
                f"{cls.rast6}=if(row() == 1 && col() == 1, 10, if(row() == 2 "
                "&& col() == 2, -10, rand(-10.0,10)))"
            ),
            seed=1,
            quiet=True,
        )
        cls.tmp_rasts.append(cls.rast6)
        cls.runModule(
            "r.support", _map=cls.rast6, semantic_label="GRASS_RND5", quiet=True
        )
        # An empty raster
        cls.rast7 = grass.tempname(10)
        cls.runModule("r.mapcalc", expression=f"{cls.rast7}=null()", quiet=True)
        cls.tmp_rasts.append(cls.rast7)
        cls.runModule(
            "r.support", _map=cls.rast7, semantic_label="GRASS_RND7", quiet=True
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
        # Range test group
        cls.group4 = grass.tempname(10)
        cls.runModule(
            "i.group", group=cls.group4, _input=(cls.rast5, cls.rast6), quiet=True
        )
        cls.tmp_groups.append(cls.group4)
        # A group with empty raster
        cls.group5 = grass.tempname(10)
        cls.runModule(
            "i.group", group=cls.group5, _input=(cls.rast6, cls.rast7), quiet=True
        )
        cls.tmp_groups.append(cls.group5)

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

    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
    def test_empty_group(self):
        """Empty imagery group handling"""
        sigfile = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group1,
            trainingmap=self.rast1,
            signaturefile=sigfile,
            quiet=True,
        )
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn(self.group1, isvm.outputs.stderr)

    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
    def test_wrong_sigfile_mapset(self):
        """Attempt to use FQ signature file name with not current mapset"""
        sigfile = grass.tempname(10)
        mapset = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            trainingmap=self.rast1,
            signaturefile=f"{sigfile}@{mapset}",
            quiet=True,
        )
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn(mapset, isvm.outputs.stderr)

    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
    def test_wrong_svm_param(self):
        """Attempt to use invalid SVM parameters"""
        sigfile = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            trainingmap=self.rast1,
            signaturefile=sigfile,
            eps=-1,
            quiet=True,
        )
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn("eps", isvm.outputs.stderr)

    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
    def test_creation_of_misc_files(self):
        """Validate creation of category, history and colour files"""
        sigfile = grass.tempname(10)
        csigdir = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(csigdir, I_SIGFILE_TYPE_LIBSVM)
        sigdir = utils.decode(csigdir.value)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            trainingmap=self.rast1,
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

    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
    def test_dont_fail_if_misc_files_missing(self):
        """Colour file is missing but it should not cause a failure"""
        sigfile = grass.tempname(10)
        csigdir = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(csigdir, I_SIGFILE_TYPE_LIBSVM)
        sigdir = utils.decode(csigdir.value)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group3,
            trainingmap=self.rast4,
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

    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
    def test_rescaling(self):
        """Raster values should be rescaled"""
        sigfile = grass.tempname(10)
        csigdir = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(csigdir, I_SIGFILE_TYPE_LIBSVM)
        sigdir = utils.decode(csigdir.value)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group4,
            trainingmap=self.rast4,
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
        G_file_name_misc(cpath, sigdir, "scale", sigfile, self.mapset_name)
        misc_file = utils.decode(cpath.value)
        self.assertTrue(os.path.isfile(misc_file))
        with open(misc_file) as rf:
            lines = rf.readlines()
            M, R = lines[0].strip().split(" ")
            self.assertTrue(float(M) > -1 and float(M) < 1)
            self.assertTrue(float(R) <= 2)
            M, R = lines[1].strip().split(" ")
            self.assertTrue(float(M) > -1 and float(M) < 1)
            self.assertTrue(float(R) <= 20)

    @unittest.skipIf(shutil.which("i.svm.train") is None, "i.svm.train not found.")
    def test_fail_on_empty_raster(self):
        """One of imagery group rasters is empty"""
        sigfile = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=self.group5,
            trainingmap=self.rast4,
            signaturefile=sigfile,
            quiet=True,
        )
        self.assertModuleFail(isvm)
        self.tmp_sigs.append(sigfile)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn("range", isvm.outputs.stderr)


if __name__ == "__main__":
    test()
