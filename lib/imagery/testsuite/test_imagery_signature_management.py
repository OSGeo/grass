"""Test of imagery library signature management functionality

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""

import os
import shutil
import ctypes

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows

from grass.script.core import tempname
import grass.script as gs
from grass.pygrass import utils
from grass.pygrass.gis import Mapset, make_mapset
from pathlib import Path

from grass.lib.gis import (
    G_mapset_path,
    G_make_mapset,
    G_reset_mapsets,
    GNAME_MAX,
    HOST_DIRSEP,
)
from grass.lib.imagery import (
    I_SIGFILE_TYPE_SIG,
    I_SIGFILE_TYPE_SIGSET,
    I_SIGFILE_TYPE_LIBSVM,
    I_find_signature,
    I_signatures_remove,
    I_signatures_copy,
    I_signatures_rename,
    I_signatures_list_by_type,
    I_free_signatures_list,
    I_get_signatures_dir,
    I_make_signatures_dir,
)


class GetSignaturesDirTestCase(TestCase):
    @xfail_windows
    def test_get_sig(self):
        cdir = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(cdir, I_SIGFILE_TYPE_SIG)
        self.assertEqual(utils.decode(cdir.value), f"signatures{HOST_DIRSEP}sig")

    @xfail_windows
    def test_get_sigset(self):
        cdir = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(cdir, I_SIGFILE_TYPE_SIGSET)
        self.assertEqual(utils.decode(cdir.value), f"signatures{HOST_DIRSEP}sigset")

    @xfail_windows
    def test_get_libsvm(self):
        elem = ctypes.create_string_buffer(GNAME_MAX)
        I_get_signatures_dir(elem, I_SIGFILE_TYPE_LIBSVM)
        self.assertEqual(utils.decode(elem.value), f"signatures{HOST_DIRSEP}libsvm")


class MakeSignaturesDirTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.org_mapset = Mapset()
        cls.tmp_mapset_name = tempname(10)
        make_mapset(mapset=cls.tmp_mapset_name)
        cls.tmp_mapset = Mapset(mapset=cls.tmp_mapset_name)
        cls.tmp_mapset.current()
        cls.tmp_mapset_path = cls.tmp_mapset.path()

    @classmethod
    def tearDownClass(cls):
        cls.org_mapset.current()
        shutil.rmtree(cls.tmp_mapset_path, ignore_errors=True)

    def test_make_sig(self):
        I_make_signatures_dir(I_SIGFILE_TYPE_SIG)
        self.assertTrue(
            os.path.isdir(os.path.join(self.tmp_mapset_path, "signatures", "sig"))
        )
        # There should not be any side effects of calling function multiple times
        I_make_signatures_dir(I_SIGFILE_TYPE_SIG)
        self.assertTrue(
            os.path.isdir(os.path.join(self.tmp_mapset_path, "signatures", "sig"))
        )

    def test_make_sigset(self):
        I_make_signatures_dir(I_SIGFILE_TYPE_SIGSET)
        self.assertTrue(
            os.path.isdir(os.path.join(self.tmp_mapset_path, "signatures", "sigset"))
        )
        # There should not be any side effects of calling function multiple times
        I_make_signatures_dir(I_SIGFILE_TYPE_SIGSET)
        self.assertTrue(
            os.path.isdir(os.path.join(self.tmp_mapset_path, "signatures", "sigset"))
        )

    def test_make_libsvm(self):
        I_make_signatures_dir(I_SIGFILE_TYPE_LIBSVM)
        self.assertTrue(
            os.path.isdir(os.path.join(self.tmp_mapset_path, "signatures", "libsvm"))
        )
        # There should not be any side effects of calling function multiple times
        I_make_signatures_dir(I_SIGFILE_TYPE_LIBSVM)
        self.assertTrue(
            os.path.isdir(os.path.join(self.tmp_mapset_path, "signatures", "libsvm"))
        )


class SignaturesRemoveTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigdirs = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/libsvm/", exist_ok=True)

    @classmethod
    def tearDownClass(cls):
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_remove_existing_sig(self):
        # This test will fail if run in PERMANENT!
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sig_dir1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        with open(sigfile_name1, "a"):
            pass
        sig_name2 = tempname(10)
        sig_dir2 = f"{self.mpath}/signatures/sig/{sig_name2}"
        os.makedirs(sig_dir2)
        self.sigdirs.append(sig_dir2)
        sigfile_name2 = f"{sig_dir2}/sig"
        with open(sigfile_name2, "a"):
            pass
        sig_name3 = tempname(10)
        sig_dir3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        os.makedirs(sig_dir3)
        self.sigdirs.append(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        with open(sigfile_name3, "a"):
            pass
        # Try to remove with wrong type
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIGSET, sig_name2)
        self.assertEqual(ret, 1)
        # Try to remove with wrong mapset
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIG, f"{sig_name2}@PERMANENT")
        self.assertEqual(ret, 1)
        # Should be still present
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Now remove with correct type
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 0)
        # removed should be gone
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Others should remain
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_nonexisting_sig(self):
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sig_dir1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        with open(sigfile_name1, "a"):
            pass
        sig_name2 = tempname(10)
        # Do not create sig_name2 matching file
        sig_name3 = tempname(10)
        sig_dir3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        os.makedirs(sig_dir3)
        self.sigdirs.append(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        with open(sigfile_name3, "a"):
            pass
        # Now remove one (should fail as file is absent)
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 1)
        # removed should be still absent
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # All others should remain
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_existing_sigset(self):
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sig_dir1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        with open(sigfile_name1, "a"):
            pass
        sig_name2 = tempname(10)
        sig_dir2 = f"{self.mpath}/signatures/sigset/{sig_name2}"
        os.makedirs(sig_dir2)
        self.sigdirs.append(sig_dir2)
        sigfile_name2 = f"{sig_dir2}/sig"
        with open(sigfile_name2, "a"):
            pass
        sig_name3 = tempname(10)
        sig_dir3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        os.makedirs(sig_dir3)
        self.sigdirs.append(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        with open(sigfile_name3, "a"):
            pass
        # Try to remove with wrong type
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 1)
        # Try to remove with wrong mapset
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIGSET, f"{sig_name2}@PERMANENT")
        self.assertEqual(ret, 1)
        # Should be still present
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Now remove with correct type
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIGSET, sig_name2)
        self.assertEqual(ret, 0)
        # removed should be gone
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Others should remain
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_nonexisting_sigset(self):
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sig_dir1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        with open(sigfile_name1, "a"):
            pass
        sig_name2 = tempname(10)
        # Do not create sig_name2 matching file
        sig_name3 = tempname(10)
        sig_dir3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        os.makedirs(sig_dir3)
        self.sigdirs.append(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        with open(sigfile_name3, "a"):
            pass
        # Now remove one (should fail as file doesn't exist)
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIGSET, sig_name2)
        self.assertEqual(ret, 1)
        # removed should be still absent
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # All others should remain
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_existing_libsvm(self):
        # This test will fail if run in PERMANENT!
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sig_dir1 = f"{self.mpath}/signatures/libsvm/{sig_name1}"
        os.makedirs(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        with open(sigfile_name1, "a"):
            pass
        self.sigdirs.append(sig_dir1)
        sig_name2 = tempname(10)
        sig_dir2 = f"{self.mpath}/signatures/libsvm/{sig_name2}"
        os.makedirs(sig_dir2)
        sigfile_name2 = f"{sig_dir2}/sig"
        with open(sigfile_name2, "a"):
            pass
        self.sigdirs.append(sig_dir2)
        sig_name3 = tempname(10)
        sig_dir3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        os.makedirs(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        with open(sigfile_name3, "a"):
            pass
        self.sigdirs.append(sig_dir3)
        # Try to remove with wrong type
        ret = I_signatures_remove(I_SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 1)
        # Try to remove with wrong mapset
        ret = I_signatures_remove(I_SIGFILE_TYPE_LIBSVM, f"{sig_name2}@PERMANENT")
        self.assertEqual(ret, 1)
        # Should be still present
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Now remove with correct type
        ret = I_signatures_remove(I_SIGFILE_TYPE_LIBSVM, sig_name2)
        self.assertEqual(ret, 0)
        # removed should be gone
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Others should remain
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_nonexisting_libsvm(self):
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sig_dir1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        os.makedirs(sig_dir1)
        sigfile_name1 = f"{sig_dir1}/sig"
        with open(sigfile_name1, "a"):
            pass
        self.sigdirs.append(sig_dir1)
        sig_name2 = tempname(10)
        # Do not create sig_name2 matching file
        sig_name3 = tempname(10)
        sig_dir3 = f"{self.mpath}/signatures/libsvm/{sig_name3}"
        os.makedirs(sig_dir3)
        sigfile_name3 = f"{sig_dir3}/sig"
        with open(sigfile_name3, "a"):
            pass
        self.sigdirs.append(sig_dir3)
        # Now remove one (should fail as file is absent)
        ret = I_signatures_remove(I_SIGFILE_TYPE_LIBSVM, sig_name2)
        self.assertEqual(ret, 1)
        # removed should be still absent
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # All others should remain
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)


class SignaturesCopyTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigdirs = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/libsvm/", exist_ok=True)
        # A mapset with a random name
        cls.src_mapset_name = tempname(10)
        G_make_mapset(None, None, cls.src_mapset_name)
        cls.src_mapset_path = (
            cls.mpath.rsplit("/", maxsplit=1)[0] + "/" + cls.src_mapset_name
        )
        # Create fake signature files
        os.makedirs(f"{cls.src_mapset_path}/signatures/sig/")
        cls.src_sig = tempname(10)
        cls.src_sig_dir = f"{cls.src_mapset_path}/signatures/sig/{cls.src_sig}"
        os.makedirs(cls.src_sig_dir)
        cls.sigdirs.append(cls.src_sig_dir)
        Path(f"{cls.src_sig_dir}/sig").write_text("A sig file")
        os.makedirs(f"{cls.src_mapset_path}/signatures/sigset/")
        cls.src_sigset = tempname(10)
        cls.src_sigset_dir = f"{cls.src_mapset_path}/signatures/sigset/{cls.src_sigset}"
        os.makedirs(cls.src_sigset_dir)
        cls.sigdirs.append(cls.src_sigset_dir)
        Path(f"{cls.src_sigset_dir}/sig").write_text("A sigset file")
        os.makedirs(f"{cls.src_mapset_path}/signatures/libsvm/")
        cls.src_libsvm = tempname(10)
        cls.src_libsvm_dir = f"{cls.src_mapset_path}/signatures/libsvm/{cls.src_libsvm}"
        os.makedirs(cls.src_libsvm_dir)
        cls.sigdirs.append(cls.src_libsvm_dir)
        Path(f"{cls.src_libsvm_dir}/sig").write_text("A libsvm file")

    @classmethod
    def tearDownClass(cls):
        # Remove random mapset created during setup
        shutil.rmtree(cls.src_mapset_path, ignore_errors=True)
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_copy_to_wrong_mapset(self):
        rnd_name = "{0}@{0}".format(tempname(10))
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_SIG, tempname(10), self.mapset_name, rnd_name
        )
        self.assertEqual(ret, 1)

    def test_sig_does_not_exist(self):
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_SIG, tempname(10), self.mapset_name, tempname(10)
        )
        self.assertEqual(ret, 1)

    def test_sigset_does_not_exist(self):
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_SIGSET, tempname(10), self.mapset_name, tempname(10)
        )
        self.assertEqual(ret, 1)

    def test_libsvm_does_not_exist(self):
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_LIBSVM, tempname(10), self.mapset_name, tempname(10)
        )
        self.assertEqual(ret, 1)

    def test_success_unqualified_sig(self):
        dst = tempname(10)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, self.src_sig, self.src_mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_SIG, self.src_sig, self.src_mapset_name, dst
        )
        self.sigdirs.append(f"{self.mpath}/signatures/sig/{dst}")
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/sig/{dst}/sig"))

    def test_success_fq_sig(self):
        dst_name = tempname(10)
        self.sigdirs.append(f"{self.mpath}/signatures/sig/{dst_name}")
        dst = dst_name + "@" + self.mapset_name
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, self.src_sig, self.src_mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_SIG,
            self.src_sig + "@" + self.src_mapset_name,
            self.src_mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/sig/{dst_name}/sig"))

    def test_success_unqualified_sigset(self):
        dst = tempname(10)
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(
            I_SIGFILE_TYPE_SIGSET, self.src_sigset, self.src_mapset_name
        )
        self.assertTrue(ret)
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_SIGSET, self.src_sigset, self.src_mapset_name, dst
        )
        self.sigdirs.append(f"{self.mpath}/signatures/sigset/{dst}")
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/sigset/{dst}/sig"))

    def test_success_fq_sigset(self):
        dst_name = tempname(10)
        self.sigdirs.append(f"{self.mpath}/signatures/sigset/{dst_name}")
        dst = dst_name + "@" + self.mapset_name
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(
            I_SIGFILE_TYPE_SIGSET, self.src_sigset, self.src_mapset_name
        )
        self.assertTrue(ret)
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_SIGSET,
            self.src_sigset + "@" + self.src_mapset_name,
            self.src_mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(
            os.path.isfile(f"{self.mpath}/signatures/sigset/{dst_name}/sig")
        )

    def test_success_unqualified_libsvm(self):
        dst = tempname(10)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(
            I_SIGFILE_TYPE_LIBSVM, self.src_libsvm, self.src_mapset_name
        )
        self.assertTrue(ret)
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_LIBSVM, self.src_libsvm, self.src_mapset_name, dst
        )
        self.sigdirs.append(f"{self.mpath}/signatures/libsvm/{dst}")
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/libsvm/{dst}/sig"))

    def test_success_fq_libsvm(self):
        dst = tempname(10)
        dst_dir = f"{self.mpath}/signatures/libsvm/{dst}"
        self.sigdirs.append(dst_dir)
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(
            I_SIGFILE_TYPE_LIBSVM, self.src_libsvm, self.src_mapset_name
        )
        self.assertTrue(ret)
        ret = I_signatures_copy(
            I_SIGFILE_TYPE_LIBSVM,
            self.src_libsvm + "@" + self.src_mapset_name,
            self.src_mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{dst_dir}/sig"))


class SignaturesRenameTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigdirs = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/libsvm/", exist_ok=True)

    @classmethod
    def tearDownClass(cls):
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_rename_from_wrong_mapset(self):
        rnd_name = "{0}@{0}".format(tempname(10))
        ret = I_signatures_rename(I_SIGFILE_TYPE_SIG, rnd_name, tempname(10))
        self.assertEqual(ret, 1)

    def test_rename_to_wrong_mapset(self):
        rnd_name = "{0}@{0}".format(tempname(10))
        ret = I_signatures_rename(I_SIGFILE_TYPE_SIG, tempname(10), rnd_name)
        self.assertEqual(ret, 1)

    def test_sig_does_not_exist(self):
        ret = I_signatures_rename(I_SIGFILE_TYPE_SIG, tempname(10), tempname(10))
        self.assertEqual(ret, 1)

    def test_sigset_does_not_exist(self):
        ret = I_signatures_rename(I_SIGFILE_TYPE_SIGSET, tempname(10), tempname(10))
        self.assertEqual(ret, 1)

    def test_libsvm_does_not_exist(self):
        ret = I_signatures_rename(I_SIGFILE_TYPE_LIBSVM, tempname(10), tempname(10))
        self.assertEqual(ret, 1)

    def test_success_unqualified_sig(self):
        src_sig = tempname(10)
        sig_dir = f"{self.mpath}/signatures/sig/{src_sig}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sig file")
        dst = tempname(10)
        self.sigdirs.append(f"{self.mpath}/signatures/sig/{dst}")
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, src_sig, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(I_SIGFILE_TYPE_SIG, src_sig, dst)
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/sig/{dst}/sig"))

    def test_success_fq_sig(self):
        src_sig = tempname(10)
        sig_dir = f"{self.mpath}/signatures/sig/{src_sig}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sig file")
        dst_name = tempname(10)
        self.sigdirs.append(f"{self.mpath}/signatures/sig/{dst_name}")
        dst = dst_name + "@" + self.mapset_name
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, src_sig, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(
            I_SIGFILE_TYPE_SIG,
            src_sig + "@" + self.mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/sig/{dst_name}/sig"))

    def test_success_unqualified_sigset(self):
        src_sigset = tempname(10)
        sig_dir = f"{self.mpath}/signatures/sigset/{src_sigset}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sigset file")
        dst = tempname(10)
        self.sigdirs.append(f"{self.mpath}/signatures/sigset/{dst}")
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, src_sigset, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(I_SIGFILE_TYPE_SIGSET, src_sigset, dst)
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/sigset/{dst}/sig"))

    def test_success_fq_sigset(self):
        src_sigset = tempname(10)
        sig_dir = f"{self.mpath}/signatures/sigset/{src_sigset}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sigset file")
        dst_name = tempname(10)
        self.sigdirs.append(f"{self.mpath}/signatures/sigset/{dst_name}")
        dst = dst_name + "@" + self.mapset_name
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, src_sigset, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(
            I_SIGFILE_TYPE_SIGSET,
            src_sigset + "@" + self.mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(
            os.path.isfile(f"{self.mpath}/signatures/sigset/{dst_name}/sig")
        )

    def test_success_unqualified_libsvm(self):
        src_sig = tempname(10)
        sig_dir = f"{self.mpath}/signatures/libsvm/{src_sig}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A libsvm file")
        dst = tempname(10)
        self.sigdirs.append(f"{self.mpath}/signatures/libsvm/{dst}")
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, src_sig, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(I_SIGFILE_TYPE_LIBSVM, src_sig, dst)
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{self.mpath}/signatures/libsvm/{dst}/sig"))

    def test_success_fq_libsvm(self):
        src_sig = tempname(10)
        sig_dir = f"{self.mpath}/signatures/libsvm/{src_sig}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A libsvm file")
        dst = tempname(10)
        dst_dir = f"{self.mpath}/signatures/libsvm/{dst}"
        self.sigdirs.append(dst_dir)
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, src_sig, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(
            I_SIGFILE_TYPE_LIBSVM,
            src_sig + "@" + self.mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(I_SIGFILE_TYPE_LIBSVM, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        self.assertTrue(os.path.isfile(f"{dst_dir}/sig"))


class SignaturesListByTypeTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.list_ptr = ctypes.POINTER(ctypes.c_char_p)
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigdirs = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/libsvm/", exist_ok=True)
        # A mapset with a random name
        cls.rnd_mapset_name = tempname(10)
        G_make_mapset(None, None, cls.rnd_mapset_name)
        cls.rnd_mapset_path = (
            cls.mpath.rsplit("/", maxsplit=1)[0] + "/" + cls.rnd_mapset_name
        )
        os.makedirs(f"{cls.rnd_mapset_path}/signatures/sig/")
        os.makedirs(f"{cls.rnd_mapset_path}/signatures/sigset/")
        os.makedirs(f"{cls.rnd_mapset_path}/signatures/libsvm/")

    @classmethod
    def tearDownClass(cls):
        # Remove random mapset created during setup
        shutil.rmtree(cls.rnd_mapset_path, ignore_errors=True)
        for d in cls.sigdirs:
            shutil.rmtree(d, ignore_errors=True)

    def test_no_sigs_at_all(self):
        # There should be no signatures in the mapset with random
        # name and thus function call should return 0 sized list
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        self.assertEqual(ret, 0)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIGSET, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        self.assertEqual(ret, 0)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_LIBSVM, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        self.assertEqual(ret, 0)
        I_free_signatures_list(ret, ctypes.byref(sig_list))

    def test_sig_in_different_mapset(self):
        # Should return 0 signatures from a different mapset
        # Sig type
        local_sig = tempname(10)
        sig_dir = f"{self.mpath}/signatures/sig/{local_sig}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sig file")
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        shutil.rmtree(sig_dir)
        self.assertEqual(ret, 0)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        # SigSet type
        local_sigset = tempname(10)
        sig_dir = f"{self.mpath}/signatures/sigset/{local_sigset}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sigset file")
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIGSET, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        shutil.rmtree(sig_dir)
        self.assertEqual(ret, 0)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        # Libsvm type
        local_sig = tempname(10)
        sig_dir = f"{self.mpath}/signatures/libsvm/{local_sig}"
        os.makedirs(sig_dir)
        sig_file = f"{sig_dir}/sig"
        self.sigdirs.append(sig_dir)
        Path(sig_file).write_text("A libsvm file")
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_LIBSVM, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        os.remove(sig_file)
        self.assertEqual(ret, 0)
        I_free_signatures_list(ret, ctypes.byref(sig_list))

    def test_single_sig(self):
        # Case when only a single signature file is present
        # Sig type
        rnd_sig = tempname(10)
        sig_dir = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sig file")
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        shutil.rmtree(sig_dir)
        self.assertEqual(ret, 1)
        val = utils.decode(sig_list[0])
        self.assertEqual(val, f"{rnd_sig}@{self.rnd_mapset_name}")
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        # SigSet type
        # SigSet equals sig. Just testing branching inside.
        rnd_sigset = tempname(10)
        sig_dir = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sigset}"
        os.makedirs(sig_dir)
        self.sigdirs.append(sig_dir)
        Path(f"{sig_dir}/sig").write_text("A sigset file")
        sigset_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIGSET, self.rnd_mapset_name, ctypes.byref(sigset_list)
        )
        shutil.rmtree(sig_dir)
        self.assertEqual(ret, 1)
        val = utils.decode(sigset_list[0])
        self.assertEqual(val, f"{rnd_sigset}@{self.rnd_mapset_name}")
        I_free_signatures_list(ret, ctypes.byref(sigset_list))
        # libsvm type
        rnd_sig = tempname(10)
        sig_dir = f"{self.rnd_mapset_path}/signatures/libsvm/{rnd_sig}"
        os.makedirs(sig_dir)
        sig_file = f"{sig_dir}/sig"
        Path(sig_file).write_text("A libsvm file")
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_LIBSVM, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        shutil.rmtree(sig_dir)
        self.assertEqual(ret, 1)
        val = utils.decode(sig_list[0])
        self.assertEqual(val, f"{rnd_sig}@{self.rnd_mapset_name}")
        I_free_signatures_list(ret, ctypes.byref(sig_list))

    def test_multiple_sigs(self):
        # Should result into a multiple sigs returned
        # Sig type
        rnd_sig1 = tempname(10)
        sig_dir1 = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        Path(f"{sig_dir1}/sig").write_text("A sig file")
        rnd_sig2 = tempname(10)
        sig_dir2 = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig2}"
        os.makedirs(sig_dir2)
        self.sigdirs.append(sig_dir2)
        Path(f"{sig_dir2}/sig").write_text("A sig file")
        # POINTER(POINTER(c_char))
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        shutil.rmtree(sig_dir1)
        shutil.rmtree(sig_dir2)
        self.assertEqual(ret, 2)
        golden = (
            f"{rnd_sig1}@{self.rnd_mapset_name}",
            f"{rnd_sig2}@{self.rnd_mapset_name}",
        )
        self.assertIn(utils.decode(sig_list[0]), golden)
        self.assertIn(utils.decode(sig_list[1]), golden)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        # SigSet type
        rnd_sigset1 = tempname(10)
        sig_dir1 = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sigset1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        Path(f"{sig_dir1}/sig").write_text("A sigset file")
        rnd_sigset2 = tempname(10)
        sig_dir2 = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sigset2}"
        os.makedirs(sig_dir2)
        self.sigdirs.append(sig_dir2)
        Path(f"{sig_dir2}/sig").write_text("A sigset file")
        sigset_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIGSET, self.rnd_mapset_name, ctypes.byref(sigset_list)
        )
        shutil.rmtree(sig_dir1)
        shutil.rmtree(sig_dir2)
        self.assertEqual(ret, 2)
        golden = (
            f"{rnd_sigset1}@{self.rnd_mapset_name}",
            f"{rnd_sigset2}@{self.rnd_mapset_name}",
        )
        self.assertIn(utils.decode(sigset_list[0]), golden)
        self.assertIn(utils.decode(sigset_list[1]), golden)
        I_free_signatures_list(ret, ctypes.byref(sigset_list))
        # libsvm type
        rnd_sig1 = tempname(10)
        sig_dir1 = f"{self.rnd_mapset_path}/signatures/libsvm/{rnd_sig1}"
        os.makedirs(sig_dir1)
        sig_file1 = f"{sig_dir1}/sig"
        Path(sig_file1).write_text("A libsvm file")
        rnd_sig2 = tempname(10)
        sig_dir2 = f"{self.rnd_mapset_path}/signatures/libsvm/{rnd_sig2}"
        os.makedirs(sig_dir2)
        sig_file2 = f"{sig_dir2}/sig"
        Path(sig_file2).write_text("A libsvm file")
        # POINTER(POINTER(c_char))
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_LIBSVM, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        shutil.rmtree(sig_dir1)
        shutil.rmtree(sig_dir2)
        self.assertEqual(ret, 2)
        golden = (
            f"{rnd_sig1}@{self.rnd_mapset_name}",
            f"{rnd_sig2}@{self.rnd_mapset_name}",
        )
        self.assertIn(utils.decode(sig_list[0]), golden)
        self.assertIn(utils.decode(sig_list[1]), golden)
        I_free_signatures_list(ret, ctypes.byref(sig_list))

    def test_multiple_sigs_multiple_mapsets(self):
        # Test searching in multiple mapsets. Identical to SIGSET case
        rnd_sig1 = tempname(10)
        sig_dir1 = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        Path(f"{sig_dir1}/sig").write_text("A sig file")
        rnd_sig2 = tempname(10)
        sig_dir2 = f"{self.mpath}/signatures/sig/{rnd_sig2}"
        os.makedirs(sig_dir2)
        self.sigdirs.append(sig_dir2)
        Path(f"{sig_dir2}/sig").write_text("A sig file")
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIG, None, ctypes.byref(sig_list)
        )
        # As temporary mapset is not in the search path, there must be
        # at least one sig file present
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 1)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        golden = (
            f"{rnd_sig1}@{self.rnd_mapset_name}",
            f"{rnd_sig2}@{self.mapset_name}",
        )
        self.assertIn(golden[1], ret_list)
        # Temporary mapset is not in the search path:
        self.assertNotIn(golden[0], ret_list)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        # Add temporary mapset to search path and re-run test
        gs.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="add")
        # Search path is cached for this run => reset!
        G_reset_mapsets()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIG, None, ctypes.byref(sig_list)
        )
        gs.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="remove")
        G_reset_mapsets()
        shutil.rmtree(sig_dir1)
        shutil.rmtree(sig_dir2)
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 2)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        self.assertIn(golden[0], ret_list)
        self.assertIn(golden[1], ret_list)
        I_free_signatures_list(ret, ctypes.byref(sig_list))

    def test_multiple_sigsets_multiple_mapsets(self):
        # Test searching in multiple mapsets. Identical to SIG case
        rnd_sig1 = tempname(10)
        sig_dir1 = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sig1}"
        os.makedirs(sig_dir1)
        self.sigdirs.append(sig_dir1)
        Path(f"{sig_dir1}/sig").write_text("A sigset file")
        rnd_sig2 = tempname(10)
        sig_dir2 = f"{self.mpath}/signatures/sigset/{rnd_sig2}"
        os.makedirs(sig_dir2)
        self.sigdirs.append(sig_dir2)
        Path(f"{sig_dir2}/sig").write_text("A sigset file")
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIGSET, None, ctypes.byref(sig_list)
        )
        # As temporary mapset is not in the search path, there must be
        # at least one sig file present
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 1)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        golden = (
            f"{rnd_sig1}@{self.rnd_mapset_name}",
            f"{rnd_sig2}@{self.mapset_name}",
        )
        self.assertIn(golden[1], ret_list)
        # Temporary mapset is not in the search path:
        self.assertNotIn(golden[0], ret_list)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        # Add temporary mapset to search path and re-run test
        gs.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="add")
        # Search path is cached for this run => reset!
        G_reset_mapsets()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_SIGSET, None, ctypes.byref(sig_list)
        )
        gs.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="remove")
        G_reset_mapsets()
        shutil.rmtree(sig_dir1)
        shutil.rmtree(sig_dir2)
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 2)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        self.assertIn(golden[0], ret_list)
        self.assertIn(golden[1], ret_list)
        I_free_signatures_list(ret, ctypes.byref(sig_list))

    def test_multiple_libsvms_multiple_mapsets(self):
        # Test searching in multiple mapsets. Identical to SIG and SIGSET case
        rnd_sig1 = tempname(10)
        sig_dir1 = f"{self.rnd_mapset_path}/signatures/libsvm/{rnd_sig1}"
        os.makedirs(sig_dir1)
        sig_file1 = f"{sig_dir1}/sig"
        Path(sig_file1).write_text("A libsvm file")
        rnd_sig2 = tempname(10)
        sig_dir2 = f"{self.mpath}/signatures/libsvm/{rnd_sig2}"
        os.makedirs(sig_dir2)
        sig_file2 = f"{sig_dir2}/sig"
        Path(sig_file2).write_text("A libsvm file")
        self.sigdirs.append(sig_dir2)
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_LIBSVM, None, ctypes.byref(sig_list)
        )
        # As temporary mapset is not in the search path, there must be
        # at least one sig file present
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 1)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        golden = (
            f"{rnd_sig1}@{self.rnd_mapset_name}",
            f"{rnd_sig2}@{self.mapset_name}",
        )
        self.assertIn(golden[1], ret_list)
        # Temporary mapset is not in the search path:
        self.assertNotIn(golden[0], ret_list)
        I_free_signatures_list(ret, ctypes.byref(sig_list))
        # Add temporary mapset to search path and re-run test
        gs.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="add")
        # Search path is cached for this run => reset!
        G_reset_mapsets()
        ret = I_signatures_list_by_type(
            I_SIGFILE_TYPE_LIBSVM, None, ctypes.byref(sig_list)
        )
        gs.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="remove")
        G_reset_mapsets()
        shutil.rmtree(sig_dir1)
        shutil.rmtree(sig_dir2)
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 2)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        self.assertIn(golden[0], ret_list)
        self.assertIn(golden[1], ret_list)
        I_free_signatures_list(ret, ctypes.byref(sig_list))


if __name__ == "__main__":
    test()
