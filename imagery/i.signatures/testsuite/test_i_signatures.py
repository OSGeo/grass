"""Test of imagery signature management module i.signatures

@author Maris Nartiss

@copyright 2023 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
import os
import shutil
import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import tempname
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import (
    G_mapset_path,
)


class PrintSignaturesTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigdirs = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
        cls.sig_name1 = tempname(10)
        sig_dir1 = f"{cls.mpath}/signatures/sigset/{cls.sig_name1}"
        os.makedirs(sig_dir1)
        cls.sigdirs.append(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        open(sigfile_name1, "a").close()
        cls.sig_name2 = tempname(10)
        sig_dir2 = f"{cls.mpath}/signatures/sig/{cls.sig_name2}"
        os.makedirs(sig_dir2)
        cls.sigdirs.append(sig_dir2)
        sigfile_name2 = f"{sig_dir2}/sig"
        open(sigfile_name2, "a").close()

    @classmethod
    def tearDownClass(cls):
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_print_all_plain(self):
        """
        If no signature file type is specified, it should print all of them
        """
        i_sig = SimpleModule("i.signatures")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        self.assertIn(self.sig_name1, i_sig.outputs.stdout)
        self.assertIn(self.sig_name2, i_sig.outputs.stdout)

    def test_print_type_plain(self):
        """
        If a type is specified, only signatures of matching type should be printed
        """
        # Case for sig
        i_sig = SimpleModule("i.signatures", type="sig")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name1, i_sig.outputs.stdout)
        self.assertIn(self.sig_name2, i_sig.outputs.stdout)
        # Case for sigset
        i_sig = SimpleModule("i.signatures", type="sigset")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        self.assertIn(self.sig_name1, i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name2, i_sig.outputs.stdout)

    def test_print_all_json(self):
        """
        If no signature file type is specified, it should print all of them
        """
        i_sig = SimpleModule("i.signatures", format="json")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        json_out = json.loads(i_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name2}@{self.mapset_name}", json_out["sig"])
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sigset"])

    def test_print_type_json(self):
        """
        If a type is specified, only signatures of matching type should be printed
        """
        # Case for sig
        i_sig = SimpleModule("i.signatures", type="sig", format="json")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        json_out = json.loads(i_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name2}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn("sigset", json_out.keys())
        # Case for sigset
        i_sig = SimpleModule("i.signatures", type="sigset", format="json")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        json_out = json.loads(i_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sigset"])
        self.assertNotIn("sig", json_out.keys())


if __name__ == "__main__":
    test()
