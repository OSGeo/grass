"""
Name:      i.svm.predict input & output tests
Purpose:   Validates user input validation code and output generation

Author:    Maris Nartiss
Copyright: (C) 2023 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import unittest
import shutil

from grass.script import core as grass
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.pygrass.gis import Mapset
from grass.lib.imagery import (
    I_SIGFILE_TYPE_LIBSVM,
    I_signatures_remove,
)


class IOValidationTest(TestCase):
    """Test input validation and output generation with i.svm.predict"""

    @classmethod
    @unittest.skipIf(shutil.which("i.svm.predict") is None, "i.svm.predict not found.")
    def setUpClass(cls):
        cls.tmp_rasts = []
        cls.tmp_groups = []
        cls.mapset_name = Mapset().name
        # Small region for small testing rasters
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)
        cls.rastt = grass.tempname(10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.rastt}=rand(0.0,1)", seed=1, quiet=True
        )
        cls.tmp_rasts.append(cls.rastt)
        cls.runModule("r.colors", _map=cls.rastt, color="grey", quiet=True)
        cls.runModule(
            "r.support", _map=cls.rastt, semantic_label="GRASS_RNDT", quiet=True
        )
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
            "i.group", group=cls.group3, _input=(cls.rast1, cls.rast3), quiet=True
        )
        cls.tmp_groups.append(cls.group3)
        # A imagery group with different semantic label count
        cls.group4 = grass.tempname(10)
        cls.runModule("i.group", group=cls.group4, _input=(cls.rast3), quiet=True)
        cls.tmp_groups.append(cls.group4)
        cls.group5 = grass.tempname(10)
        cls.runModule(
            "i.group",
            group=cls.group5,
            _input=(cls.rast1, cls.rast3, cls.rastt),
            quiet=True,
        )
        cls.tmp_groups.append(cls.group5)
        # Generate a signature file
        cls.sig1 = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.train",
            group=cls.group3,
            trainingmap=cls.rastt,
            signaturefile=cls.sig1,
            quiet=True,
        )
        isvm.run()

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.del_temp_region()
        for rast in cls.tmp_rasts:
            cls.runModule("g.remove", flags="f", _type="raster", name=rast)
        for group in cls.tmp_groups:
            cls.runModule("g.remove", flags="f", _type="group", name=group)
        I_signatures_remove(I_SIGFILE_TYPE_LIBSVM, cls.sig1)

    @unittest.skipIf(shutil.which("i.svm.predict") is None, "i.svm.predict not found.")
    def test_empty_group(self):
        """Empty imagery group handling"""
        rast = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.predict",
            group=self.group1,
            output=rast,
            signaturefile=self.sig1,
            quiet=True,
        )
        self.tmp_rasts.append(rast)
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn(self.group1, isvm.outputs.stderr)

    @unittest.skipIf(shutil.which("i.svm.predict") is None, "i.svm.predict not found.")
    def test_semantic_label_mismatch1(self):
        """There are more semantic labels in the signature file than in the group"""
        rast = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.predict",
            group=self.group4,
            output=rast,
            signaturefile=self.sig1,
            quiet=True,
        )
        self.tmp_rasts.append(rast)
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertIn(
            "Imagery group does not contain a raster with a semantic label",
            isvm.outputs.stderr,
        )
        self.assertIn(
            self.rast1,
            isvm.outputs.stderr,
        )

    @unittest.skipIf(shutil.which("i.svm.predict") is None, "i.svm.predict not found.")
    def test_semantic_label_mismatch2(self):
        """There are more semantic labels in the group than in the signature file"""
        rast = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.predict",
            group=self.group5,
            output=rast,
            signaturefile=self.sig1,
            quiet=True,
        )
        self.tmp_rasts.append(rast)
        self.assertModuleFail(isvm)
        self.assertTrue(isvm.outputs.stderr)
        self.assertTrue(
            "Signature band count: 2, imagery group band count: 3"
            in isvm.outputs.stderr
        )

    @unittest.skipIf(shutil.which("i.svm.predict") is None, "i.svm.predict not found.")
    def test_prediction(self):
        """A successful run"""
        rast = grass.tempname(10)
        isvm = SimpleModule(
            "i.svm.predict",
            group=self.group3,
            output=rast,
            signaturefile=self.sig1,
            quiet=True,
        )
        self.tmp_rasts.append(rast)
        self.assertModule(isvm)
        self.assertRasterExists(rast)


if __name__ == "__main__":
    test()
