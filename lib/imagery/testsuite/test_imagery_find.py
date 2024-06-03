"""Test of imagery library file searching functionality

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
<<<<<<< HEAD

import os
import shutil
=======
import os
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import G_mapset_path
from grass.lib.imagery import (
    I_SIGFILE_TYPE_SIG,
    I_SIGFILE_TYPE_SIGSET,
<<<<<<< HEAD
    I_SIGFILE_TYPE_LIBSVM,
=======
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
    I_find_signature,
    I_find_signature2,
)


class FindSignatureTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
<<<<<<< HEAD
        cls.sigdirs = []
=======
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
<<<<<<< HEAD
        os.makedirs(f"{cls.mpath}/signatures/libsvm/", exist_ok=True)
        cls.sig_name1 = tempname(10)
        cls.sig_dir1 = f"{cls.mpath}/signatures/sigset/{cls.sig_name1}"
        os.makedirs(cls.sig_dir1)
        cls.sigdirs.append(cls.sig_dir1)
        open(f"{cls.sig_dir1}/sig", "a").close()
        cls.sig_name2 = tempname(10)
        cls.sig_dir2 = f"{cls.mpath}/signatures/sig/{cls.sig_name2}"
        os.makedirs(cls.sig_dir2)
        cls.sigdirs.append(cls.sig_dir2)
        open(f"{cls.sig_dir2}/sig", "a").close()
        cls.sig_name3 = tempname(10)
        cls.sig_dir3 = f"{cls.mpath}/signatures/libsvm/{cls.sig_name3}"
        os.makedirs(cls.sig_dir3)
        cls.sigdirs.append(cls.sig_dir3)
        open(f"{cls.sig_dir3}/sig", "a").close()

    @classmethod
    def tearDownClass(cls):
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)
=======
        cls.sig_name1 = tempname(10)
        cls.sigfile_name1 = f"{cls.mpath}/signatures/sigset/{cls.sig_name1}"
        open(cls.sigfile_name1, "a").close()
        cls.sig_name2 = tempname(10)
        cls.sigfile_name2 = f"{cls.mpath}/signatures/sig/{cls.sig_name2}"
        open(cls.sigfile_name2, "a").close()

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(cls.sigfile_name1)
            os.remove(cls.sigfile_name2)
        except OSError:
            pass
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))

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

<<<<<<< HEAD
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

=======
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
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

<<<<<<< HEAD
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

=======
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))

if __name__ == "__main__":
    test()
