"""Test of imagery library file searching functionality

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""

import shutil
from pathlib import Path

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.lib.gis import G_mapset_path
from grass.lib.imagery import (
    I_SIGFILE_TYPE_LIBSVM,
    I_SIGFILE_TYPE_SIG,
    I_SIGFILE_TYPE_SIGSET,
    I_find_signature,
    I_find_signature2,
)
from grass.pygrass import utils
from grass.pygrass.gis import Mapset
from grass.script.core import tempname


class FindSignatureTestCase(TestCase):
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
        cls.sig_name1 = tempname(10)
        cls.sig_dir1 = f"{cls.mpath}/signatures/sigset/{cls.sig_name1}"
        Path(cls.sig_dir1).mkdir(parents=True)
        cls.sigdirs.append(cls.sig_dir1)
        open(f"{cls.sig_dir1}/sig", "a").close()
        cls.sig_name2 = tempname(10)
        cls.sig_dir2 = f"{cls.mpath}/signatures/sig/{cls.sig_name2}"
        Path(cls.sig_dir2).mkdir(parents=True)
        cls.sigdirs.append(cls.sig_dir2)
        open(f"{cls.sig_dir2}/sig", "a").close()
        cls.sig_name3 = tempname(10)
        cls.sig_dir3 = f"{cls.mpath}/signatures/libsvm/{cls.sig_name3}"
        Path(cls.sig_dir3).mkdir(parents=True)
        cls.sigdirs.append(cls.sig_dir3)
        open(f"{cls.sig_dir3}/sig", "a").close()

    @classmethod
    def tearDownClass(cls):
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_find_sig(self):
        # Non existing without a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sigset with sig type should equal non existing
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, self.sig_name1, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, self.sig_name2, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, self.sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, self.sig_name2, "PERMANENT")
        self.assertFalse(ret)

    def test_find_sigset(self):
        # Non existing without a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sig with sigset type should equal non existing
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, self.sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, self.sig_name1, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, self.sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, self.sig_name1, "PERMANENT")
        self.assertFalse(ret)

    def test_find_libsvm(self):
        # Non existing without a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Libsvm with sig type should equal non existing
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, self.sig_name3, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, self.sig_name3, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, self.sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, self.sig_name3, "PERMANENT")
        self.assertFalse(ret)

    def test_find2_sig(self):
        # Non existing without a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIG, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIG, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sigset with sig type should equal non existing
        ret = I_find_signature2(I_SIGFILE_TYPE_SIG, self.sig_name1, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIG, self.sig_name2, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIG, self.sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature2(I_SIGFILE_TYPE_SIG, self.sig_name2, "PERMANENT")
        self.assertFalse(ret)

    def test_find2_sigset(self):
        # Non existing without a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIGSET, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIGSET, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sig with sigset type should equal non existing
        ret = I_find_signature2(I_SIGFILE_TYPE_SIGSET, self.sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIGSET, self.sig_name1, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_SIGSET, self.sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature2(I_SIGFILE_TYPE_SIGSET, self.sig_name1, "PERMANENT")
        self.assertFalse(ret)

    def test_find2_libsvm(self):
        # Non existing without a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_LIBSVM, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_LIBSVM, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Libsvm with sig type should equal non existing
        ret = I_find_signature2(I_SIGFILE_TYPE_SIG, self.sig_name3, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_LIBSVM, self.sig_name3, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature2(I_SIGFILE_TYPE_LIBSVM, self.sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature2(I_SIGFILE_TYPE_LIBSVM, self.sig_name3, "PERMANENT")
        self.assertFalse(ret)


if __name__ == "__main__":
    test()
