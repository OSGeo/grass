"""Test of imagery signature management module i.signatures

@author Maris Nartiss

@copyright 2023 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""

import shutil
import json
from pathlib import Path

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
        Path(f"{cls.mpath}/signatures/sig/").mkdir(exist_ok=True, parents=True)
        Path(f"{cls.mpath}/signatures/sigset/").mkdir(exist_ok=True, parents=True)
        Path(f"{cls.mpath}/signatures/libsvm/").mkdir(exist_ok=True, parents=True)
        # Fake signature of sig type
        cls.sig_name1 = tempname(10)
        sig_dir1 = f"{cls.mpath}/signatures/sig/{cls.sig_name1}"
        Path(sig_dir1).mkdir(parents=True)
        cls.sigdirs.append(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        open(sigfile_name1, "a").close()
        # Fake signature of sigset type
        cls.sig_name2 = tempname(10)
        sig_dir2 = f"{cls.mpath}/signatures/sigset/{cls.sig_name2}"
        Path(sig_dir2).mkdir(parents=True)
        cls.sigdirs.append(sig_dir2)
        sigfile_name2 = f"{sig_dir2}/sig"
        open(sigfile_name2, "a").close()
        # Fake signature of libsvm type
        cls.sig_name3 = tempname(10)
        sig_dir3 = f"{cls.mpath}/signatures/libsvm/{cls.sig_name3}"
        Path(sig_dir3).mkdir(parents=True)
        cls.sigdirs.append(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        open(sigfile_name3, "a").close()

    @classmethod
    def tearDownClass(cls):
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_print_all_plain(self):
        """
        If no signature file type is specified, it should print all of them
        """
        i_sig = SimpleModule("i.signatures", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        self.assertIn(self.sig_name1, i_sig.outputs.stdout)
        self.assertIn(self.sig_name2, i_sig.outputs.stdout)
        self.assertIn(self.sig_name3, i_sig.outputs.stdout)

    def test_print_type_plain(self):
        """
        If a type is specified, only signatures of matching type should be printed
        """
        # Case for sig
        i_sig = SimpleModule("i.signatures", type="sig", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        self.assertIn(self.sig_name1, i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name2, i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name3, i_sig.outputs.stdout)
        # Case for sigset
        i_sig = SimpleModule("i.signatures", type="sigset", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name1, i_sig.outputs.stdout)
        self.assertIn(self.sig_name2, i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name3, i_sig.outputs.stdout)
        # Case for libsvm
        i_sig = SimpleModule("i.signatures", type="libsvm", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name1, i_sig.outputs.stdout)
        self.assertNotIn(self.sig_name2, i_sig.outputs.stdout)
        self.assertIn(self.sig_name3, i_sig.outputs.stdout)

    def test_print_all_json(self):
        """
        If no signature file type is specified, it should print all of them
        """
        i_sig = SimpleModule("i.signatures", format="json", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        json_out = json.loads(i_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sig"])
        self.assertIn(f"{self.sig_name2}@{self.mapset_name}", json_out["sigset"])
        self.assertIn(f"{self.sig_name3}@{self.mapset_name}", json_out["libsvm"])

    def test_print_type_json(self):
        """
        If a type is specified, only signatures of matching type should be printed
        """
        # Case for sig
        i_sig = SimpleModule("i.signatures", type="sig", format="json", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        json_out = json.loads(i_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn("sigset", json_out.keys())
        self.assertNotIn("libsvm", json_out.keys())
        # Case for sigset
        i_sig = SimpleModule("i.signatures", type="sigset", format="json", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        json_out = json.loads(i_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name2}@{self.mapset_name}", json_out["sigset"])
        self.assertNotIn("sig", json_out.keys())
        self.assertNotIn("libsvm", json_out.keys())
        # Case for libsvm
        i_sig = SimpleModule("i.signatures", type="libsvm", format="json", flags="p")
        self.assertModule(i_sig)
        self.assertTrue(i_sig.outputs.stdout)
        json_out = json.loads(i_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name3}@{self.mapset_name}", json_out["libsvm"])
        self.assertNotIn("sig", json_out.keys())
        self.assertNotIn("sigset", json_out.keys())


class ManageSignaturesTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigdirs = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        Path(f"{cls.mpath}/signatures/sig/").mkdir(exist_ok=True, parents=True)
        Path(f"{cls.mpath}/signatures/sigset/").mkdir(exist_ok=True, parents=True)
        Path(f"{cls.mpath}/signatures/libsvm/").mkdir(exist_ok=True, parents=True)
        # sig
        cls.sig_name1 = tempname(10)
        sig_dir1 = f"{cls.mpath}/signatures/sig/{cls.sig_name1}"
        Path(sig_dir1).mkdir(parents=True)
        cls.sigdirs.append(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        open(sigfile_name1, "a").close()
        # sig
        cls.sig_name2 = tempname(10)
        sig_dir2 = f"{cls.mpath}/signatures/sig/{cls.sig_name2}"
        Path(sig_dir2).mkdir(parents=True)
        cls.sigdirs.append(sig_dir2)
        sigfile_name2 = f"{sig_dir2}/sig"
        open(sigfile_name2, "a").close()
        # sigset
        cls.sig_name3 = tempname(10)
        sig_dir3 = f"{cls.mpath}/signatures/sigset/{cls.sig_name3}"
        Path(sig_dir3).mkdir(parents=True)
        cls.sigdirs.append(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        open(sigfile_name3, "a").close()
        # sigset
        cls.sig_name4 = tempname(10)
        sig_dir4 = f"{cls.mpath}/signatures/sigset/{cls.sig_name4}"
        Path(sig_dir4).mkdir(parents=True)
        cls.sigdirs.append(sig_dir4)
        sigfile_name4 = f"{sig_dir4}/sig"
        open(sigfile_name4, "a").close()
        # libsvm
        cls.sig_name5 = tempname(10)
        sig_dir5 = f"{cls.mpath}/signatures/libsvm/{cls.sig_name5}"
        Path(sig_dir5).mkdir(parents=True)
        cls.sigdirs.append(sig_dir5)
        sigfile_name5 = f"{sig_dir5}/sig"
        open(sigfile_name5, "a").close()
        # libsvm
        cls.sig_name6 = tempname(10)
        sig_dir6 = f"{cls.mpath}/signatures/libsvm/{cls.sig_name6}"
        Path(sig_dir6).mkdir(parents=True)
        cls.sigdirs.append(sig_dir6)
        sigfile_name6 = f"{sig_dir6}/sig"
        open(sigfile_name6, "a").close()

    @classmethod
    def tearDownClass(cls):
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_copy(self):
        a_copy = tempname(10)
        # Fail if type is not provided
        i_sig = SimpleModule("i.signatures", copy=(self.sig_name1, a_copy))
        self.assertModuleFail(i_sig)

        # Do nothing if file is not found
        i_sig = SimpleModule(
            "i.signatures", type="sigset", copy=(self.sig_name1, a_copy)
        )
        self.assertModule(i_sig)
        l_sig = SimpleModule("i.signatures", format="json", flags="p")
        self.assertModule(l_sig)
        self.assertTrue(l_sig.outputs.stdout)
        json_out = json.loads(l_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["sigset"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["libsvm"])

        # If all is correct, copy should succeed
        i_sig = SimpleModule("i.signatures", type="sig", copy=(self.sig_name1, a_copy))
        self.assertModule(i_sig)
        l_sig = SimpleModule("i.signatures", format="json", flags="p")
        self.assertModule(l_sig)
        self.assertTrue(l_sig.outputs.stdout)
        json_out = json.loads(l_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sig"])
        self.assertIn(f"{a_copy}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["sigset"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["libsvm"])

    def test_rename(self):
        a_copy = tempname(10)
        # Fail if type is not provided
        i_sig = SimpleModule("i.signatures", rename=(self.sig_name2, a_copy))
        self.assertModuleFail(i_sig)

        # Do nothing if file is not found
        i_sig = SimpleModule(
            "i.signatures", type="sigset", rename=(self.sig_name2, a_copy)
        )
        self.assertModule(i_sig)
        l_sig = SimpleModule("i.signatures", format="json", flags="p")
        self.assertModule(l_sig)
        self.assertTrue(l_sig.outputs.stdout)
        json_out = json.loads(l_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name2}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["sigset"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["libsvm"])

        # If all is correct, rename should succeed
        i_sig = SimpleModule(
            "i.signatures", type="sig", rename=(self.sig_name2, a_copy)
        )
        self.assertModule(i_sig)
        l_sig = SimpleModule("i.signatures", format="json", flags="p")
        self.assertModule(l_sig)
        self.assertTrue(l_sig.outputs.stdout)
        json_out = json.loads(l_sig.outputs.stdout)
        self.assertNotIn(f"{self.sig_name2}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn(f"{self.sig_name2}@{self.mapset_name}", json_out["sigset"])
        self.assertNotIn(f"{self.sig_name2}@{self.mapset_name}", json_out["libsvm"])
        self.assertIn(f"{a_copy}@{self.mapset_name}", json_out["sig"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["sigset"])
        self.assertNotIn(f"{a_copy}@{self.mapset_name}", json_out["libsvm"])

    def test_remove(self):
        # Fail if type is not provided
        i_sig = SimpleModule("i.signatures", remove=self.sig_name3)
        self.assertModuleFail(i_sig)

        # Do nothing if file is not found
        i_sig = SimpleModule("i.signatures", type="sig", remove=self.sig_name3)
        self.assertModule(i_sig)
        l_sig = SimpleModule("i.signatures", format="json", flags="p")
        self.assertModule(l_sig)
        self.assertTrue(l_sig.outputs.stdout)
        json_out = json.loads(l_sig.outputs.stdout)
        self.assertIn(f"{self.sig_name3}@{self.mapset_name}", json_out["sigset"])
        self.assertIn(f"{self.sig_name4}@{self.mapset_name}", json_out["sigset"])
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sig"])
        self.assertIn(f"{self.sig_name5}@{self.mapset_name}", json_out["libsvm"])
        self.assertIn(f"{self.sig_name6}@{self.mapset_name}", json_out["libsvm"])

        # If all is correct, remove should succeed
        i_sig = SimpleModule("i.signatures", type="sigset", remove=self.sig_name3)
        self.assertModule(i_sig)
        l_sig = SimpleModule("i.signatures", format="json", flags="p")
        self.assertModule(l_sig)
        self.assertTrue(l_sig.outputs.stdout)
        json_out = json.loads(l_sig.outputs.stdout)
        self.assertNotIn(f"{self.sig_name3}@{self.mapset_name}", json_out["sigset"])
        self.assertIn(f"{self.sig_name4}@{self.mapset_name}", json_out["sigset"])
        self.assertIn(f"{self.sig_name1}@{self.mapset_name}", json_out["sig"])
        self.assertIn(f"{self.sig_name5}@{self.mapset_name}", json_out["libsvm"])
        self.assertIn(f"{self.sig_name6}@{self.mapset_name}", json_out["libsvm"])


if __name__ == "__main__":
    test()
