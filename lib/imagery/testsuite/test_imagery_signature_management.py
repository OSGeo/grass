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

from grass.script.core import tempname
import grass.script as grass
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import G_mapset_path, G_make_mapset, G_reset_mapsets
from grass.lib.imagery import (
    SIGFILE_TYPE_SIG,
    SIGFILE_TYPE_SIGSET,
    I_find_signature,
    I_signatures_remove,
    I_signatures_copy,
    I_signatures_rename,
    I_signatures_list_by_type,
)


class SignaturesRemoveTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigfiles = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)

    @classmethod
    def tearDownClass(cls):
        for f in cls.sigfiles:
            try:
                os.remove(f)
            except OSError:
                pass

    def test_remove_existing_sig(self):
        # This test will fail if run in PERMANENT!
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sigfile_name1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        sigfile_name2 = f"{self.mpath}/signatures/sig/{sig_name2}"
        open(sigfile_name2, "a").close()
        self.sigfiles.append(sigfile_name2)
        sig_name3 = tempname(10)
        sigfile_name3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        open(sigfile_name3, "a").close()
        self.sigfiles.append(sigfile_name3)
        # Try to remove with wrong type
        ret = I_signatures_remove(SIGFILE_TYPE_SIGSET, sig_name2)
        self.assertEqual(ret, 1)
        # Try to remove with wrong mapset
        ret = I_signatures_remove(SIGFILE_TYPE_SIG, f"{sig_name2}@PERMANENT")
        self.assertEqual(ret, 1)
        # Should be still present
        ret = I_find_signature(SIGFILE_TYPE_SIG, sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Now remove with correct type
        ret = I_signatures_remove(SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 0)
        # removed should be gone
        ret = I_find_signature(SIGFILE_TYPE_SIG, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Others should remain
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_nonexisting_sig(self):
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sigfile_name1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        # Do not create sig_name2 matching file
        sig_name3 = tempname(10)
        sigfile_name3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        open(sigfile_name3, "a").close()
        self.sigfiles.append(sigfile_name3)
        # Now remove one (should fail as file is absent)
        ret = I_signatures_remove(SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 1)
        # removed should be still absent
        ret = I_find_signature(SIGFILE_TYPE_SIG, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # All others should remain
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_existing_sigset(self):
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sigfile_name1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        sigfile_name2 = f"{self.mpath}/signatures/sigset/{sig_name2}"
        open(sigfile_name2, "a").close()
        self.sigfiles.append(sigfile_name2)
        sig_name3 = tempname(10)
        sigfile_name3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        open(sigfile_name3, "a").close()
        self.sigfiles.append(sigfile_name3)
        # Try to remove with wrong type
        ret = I_signatures_remove(SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 1)
        # Try to remove with wrong mapset
        ret = I_signatures_remove(SIGFILE_TYPE_SIGSET, f"{sig_name2}@PERMANENT")
        self.assertEqual(ret, 1)
        # Should be still present
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Now remove with correct type
        ret = I_signatures_remove(SIGFILE_TYPE_SIGSET, sig_name2)
        self.assertEqual(ret, 0)
        # removed should be gone
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Others should remain
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_remove_nonexisting_sigset(self):
        # Set up files and mark for clean-up
        sig_name1 = tempname(10)
        sigfile_name1 = f"{self.mpath}/signatures/sigset/{sig_name1}"
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        # Do not create sig_name2 matching file
        sig_name3 = tempname(10)
        sigfile_name3 = f"{self.mpath}/signatures/sig/{sig_name3}"
        open(sigfile_name3, "a").close()
        self.sigfiles.append(sigfile_name3)
        # Now remove one (should fail as file doesn't exist)
        ret = I_signatures_remove(SIGFILE_TYPE_SIGSET, sig_name2)
        self.assertEqual(ret, 1)
        # removed should be still absent
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # All others should remain
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        ret = I_find_signature(SIGFILE_TYPE_SIG, sig_name3, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)


class SignaturesCopyTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigfiles = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
        # A mapset with a random name
        cls.src_mapset_name = tempname(10)
        G_make_mapset(None, None, cls.src_mapset_name)
        cls.src_mapset_path = (
            cls.mpath.rsplit("/", maxsplit=1)[0] + "/" + cls.src_mapset_name
        )
        os.makedirs(f"{cls.src_mapset_path}/signatures/sig/")
        cls.src_sig = tempname(10)
        cls.sigfiles.append(f"{cls.src_mapset_path}/signatures/sig/{cls.src_sig}")
        f = open(cls.sigfiles[0], "w")
        f.write("A sig file")
        f.close()
        os.makedirs(f"{cls.src_mapset_path}/signatures/sigset/")
        cls.src_sigset = tempname(10)
        cls.sigfiles.append(f"{cls.src_mapset_path}/signatures/sigset/{cls.src_sigset}")
        f = open(cls.sigfiles[1], "w")
        f.write("A sigset file")
        f.close()

    @classmethod
    def tearDownClass(cls):
        # Remove random mapset created during setup
        shutil.rmtree(cls.src_mapset_path, ignore_errors=True)
        for f in cls.sigfiles:
            try:
                os.remove(f)
            except OSError:
                pass

    def test_copy_to_wrong_mapset(self):
        rnd_name = "{0}@{0}".format(tempname(10))
        ret = I_signatures_copy(
            SIGFILE_TYPE_SIG, tempname(10), self.mapset_name, rnd_name
        )
        self.assertEqual(ret, 1)

    def test_sig_does_not_exist(self):
        ret = I_signatures_copy(
            SIGFILE_TYPE_SIG, tempname(10), self.mapset_name, tempname(10)
        )
        self.assertEqual(ret, 1)

    def test_sigset_does_not_exist(self):
        ret = I_signatures_copy(
            SIGFILE_TYPE_SIGSET, tempname(10), self.mapset_name, tempname(10)
        )
        self.assertEqual(ret, 1)

    def test_success_unqualified_sig(self):
        dst = tempname(10)
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIG, self.src_sig, self.src_mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_copy(
            SIGFILE_TYPE_SIG, self.src_sig, self.src_mapset_name, dst
        )
        self.sigfiles.append(f"{self.mpath}/signatures/sig/{dst}")
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_fq_sig(self):
        dst = tempname(10)
        self.sigfiles.append(f"{self.mpath}/signatures/sig/{dst}")
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIG, self.src_sig, self.src_mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_copy(
            SIGFILE_TYPE_SIG,
            self.src_sig + "@" + self.src_mapset_name,
            self.src_mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_unqualified_sigset(self):
        dst = tempname(10)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(
            SIGFILE_TYPE_SIGSET, self.src_sigset, self.src_mapset_name
        )
        self.assertTrue(ret)
        ret = I_signatures_copy(
            SIGFILE_TYPE_SIGSET, self.src_sigset, self.src_mapset_name, dst
        )
        self.sigfiles.append(f"{self.mpath}/signatures/sigset/{dst}")
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_fq_sigset(self):
        dst = tempname(10)
        self.sigfiles.append(f"{self.mpath}/signatures/sigset/{dst}")
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(
            SIGFILE_TYPE_SIGSET, self.src_sigset, self.src_mapset_name
        )
        self.assertTrue(ret)
        ret = I_signatures_copy(
            SIGFILE_TYPE_SIGSET,
            self.src_sigset + "@" + self.src_mapset_name,
            self.src_mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)


class SignaturesRenameTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigfiles = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)

    @classmethod
    def tearDownClass(cls):
        for f in cls.sigfiles:
            try:
                os.remove(f)
            except OSError:
                pass

    def test_rename_from_wrong_mapset(self):
        rnd_name = "{0}@{0}".format(tempname(10))
        ret = I_signatures_rename(SIGFILE_TYPE_SIG, rnd_name, tempname(10))
        self.assertEqual(ret, 1)

    def test_rename_to_wrong_mapset(self):
        rnd_name = "{0}@{0}".format(tempname(10))
        ret = I_signatures_rename(SIGFILE_TYPE_SIG, tempname(10), rnd_name)
        self.assertEqual(ret, 1)

    def test_sig_does_not_exist(self):
        ret = I_signatures_rename(SIGFILE_TYPE_SIG, tempname(10), tempname(10))
        self.assertEqual(ret, 1)

    def test_sigset_does_not_exist(self):
        ret = I_signatures_rename(SIGFILE_TYPE_SIGSET, tempname(10), tempname(10))
        self.assertEqual(ret, 1)

    def test_success_unqualified_sig(self):
        src_sig = tempname(10)
        sig_file = f"{self.mpath}/signatures/sig/{src_sig}"
        self.sigfiles.append(sig_file)
        f = open(sig_file, "w")
        f.write("A sig file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append(f"{self.mpath}/signatures/sig/{dst}")
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIG, src_sig, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(SIGFILE_TYPE_SIG, src_sig, dst)
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_fq_sig(self):
        src_sig = tempname(10)
        sig_file = f"{self.mpath}/signatures/sig/{src_sig}"
        self.sigfiles.append(sig_file)
        f = open(sig_file, "w")
        f.write("A sig file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append(f"{self.mpath}/signatures/sig/{dst}")
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIG, src_sig, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(
            SIGFILE_TYPE_SIG,
            src_sig + "@" + self.mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_unqualified_sigset(self):
        src_sigset = tempname(10)
        sigset_file = f"{self.mpath}/signatures/sigset/{src_sigset}"
        self.sigfiles.append(sigset_file)
        f = open(sigset_file, "w")
        f.write("A sigset file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append(f"{self.mpath}/signatures/sigset/{dst}")
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, src_sigset, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(SIGFILE_TYPE_SIGSET, src_sigset, dst)
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_fq_sigset(self):
        src_sigset = tempname(10)
        sigset_file = f"{self.mpath}/signatures/sigset/{src_sigset}"
        self.sigfiles.append(sigset_file)
        f = open(sigset_file, "w")
        f.write("A sigset file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append(f"{self.mpath}/signatures/sigset/{dst}")
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, src_sigset, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(
            SIGFILE_TYPE_SIGSET,
            src_sigset + "@" + self.mapset_name,
            dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)


class SignaturesListByTypeTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.list_ptr = ctypes.POINTER(ctypes.c_char_p)
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigfiles = []
        # As signatures are created directly not via signature creation
        # tools, we must ensure signature directories exist
        os.makedirs(f"{cls.mpath}/signatures/sig/", exist_ok=True)
        os.makedirs(f"{cls.mpath}/signatures/sigset/", exist_ok=True)
        # A mapset with a random name
        cls.rnd_mapset_name = tempname(10)
        G_make_mapset(None, None, cls.rnd_mapset_name)
        cls.rnd_mapset_path = (
            cls.mpath.rsplit("/", maxsplit=1)[0] + "/" + cls.rnd_mapset_name
        )
        os.makedirs(f"{cls.rnd_mapset_path}/signatures/sig/")
        os.makedirs(f"{cls.rnd_mapset_path}/signatures/sigset/")

    @classmethod
    def tearDownClass(cls):
        # Remove random mapset created during setup
        shutil.rmtree(cls.rnd_mapset_path, ignore_errors=True)
        for f in cls.sigfiles:
            try:
                os.remove(f)
            except OSError:
                pass

    def test_no_sigs_at_all(self):
        # There should be no signatures in the mapset with random
        # name and thus function call should return 0 sized list
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        self.assertEqual(ret, 0)

    def test_sig_in_different_mapset(self):
        # Should return 0 signatures from a different mapset
        local_sig = tempname(10)
        sig_file = f"{self.mpath}/signatures/sig/{local_sig}"
        self.sigfiles.append(sig_file)
        f = open(sig_file, "w")
        f.write("A sig file")
        f.close()
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        os.remove(sig_file)
        self.assertEqual(ret, 0)
        local_sigset = tempname(10)
        sigset_file = f"{self.mpath}/signatures/sigset/{local_sigset}"
        self.sigfiles.append(sigset_file)
        f = open(sigset_file, "w")
        f.write("A sigset file")
        f.close()
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIGSET, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        os.remove(sigset_file)
        self.assertEqual(ret, 0)

    def test_single_sig(self):
        # Case when only a single signature file is present
        rnd_sig = tempname(10)
        sig_file = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig}"
        f = open(sig_file, "w")
        f.write("A sig file")
        f.close()
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        os.remove(sig_file)
        self.assertEqual(ret, 1)
        val = utils.decode(sig_list[0])
        self.assertEqual(val, f"{rnd_sig}@{self.rnd_mapset_name}")
        # SigSet equals sig. Just testing branching inside.
        rnd_sigset = tempname(10)
        sigset_file = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sigset}"
        f = open(sigset_file, "w")
        f.write("A sigset file")
        f.close()
        sigset_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIGSET, self.rnd_mapset_name, ctypes.byref(sigset_list)
        )
        os.remove(sigset_file)
        self.assertEqual(ret, 1)
        val = utils.decode(sigset_list[0])
        self.assertEqual(val, f"{rnd_sigset}@{self.rnd_mapset_name}")

    def test_multiple_sigs(self):
        # Should result into a multiple sigs returned
        rnd_sig1 = tempname(10)
        sig_file1 = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig1}"
        f = open(sig_file1, "w")
        f.write("A sig file")
        f.close()
        rnd_sig2 = tempname(10)
        sig_file2 = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig2}"
        f = open(sig_file2, "w")
        f.write("A sig file")
        f.close()
        # POINTER(POINTER(c_char))
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIG, self.rnd_mapset_name, ctypes.byref(sig_list)
        )
        os.remove(sig_file1)
        os.remove(sig_file2)
        self.assertEqual(ret, 2)
        golden = (
            f"{rnd_sig1}@{self.rnd_mapset_name}",
            f"{rnd_sig2}@{self.rnd_mapset_name}",
        )
        self.assertIn(utils.decode(sig_list[0]), golden)
        self.assertIn(utils.decode(sig_list[1]), golden)
        # Ditto for sigset
        rnd_sigset1 = tempname(10)
        sigset_file1 = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sigset1}"
        f = open(sigset_file1, "w")
        f.write("A sigset file")
        f.close()
        rnd_sigset2 = tempname(10)
        sigset_file2 = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sigset2}"
        f = open(sigset_file2, "w")
        f.write("A sigset file")
        f.close()
        sigset_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIGSET, self.rnd_mapset_name, ctypes.byref(sigset_list)
        )
        os.remove(sigset_file1)
        os.remove(sigset_file2)
        self.assertEqual(ret, 2)
        golden = (
            f"{rnd_sigset1}@{self.rnd_mapset_name}",
            f"{rnd_sigset2}@{self.rnd_mapset_name}",
        )
        self.assertIn(utils.decode(sigset_list[0]), golden)
        self.assertIn(utils.decode(sigset_list[1]), golden)

    def test_multiple_sigs_multiple_mapsets(self):
        # Test searching in multiple mapsets. Identical to SIGSET case
        rnd_sig1 = tempname(10)
        sig_file1 = f"{self.rnd_mapset_path}/signatures/sig/{rnd_sig1}"
        f = open(sig_file1, "w")
        f.write("A sig file")
        f.close()
        rnd_sig2 = tempname(10)
        sig_file2 = f"{self.mpath}/signatures/sig/{rnd_sig2}"
        f = open(sig_file2, "w")
        f.write("A sig file")
        f.close()
        self.sigfiles.append(sig_file2)
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(SIGFILE_TYPE_SIG, None, ctypes.byref(sig_list))
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
        # Add temporary mapset to search path and re-run test
        grass.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="add")
        # Search path is cached for this run => reset!
        G_reset_mapsets()
        ret = I_signatures_list_by_type(SIGFILE_TYPE_SIG, None, ctypes.byref(sig_list))
        grass.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="remove")
        G_reset_mapsets()
        os.remove(sig_file1)
        os.remove(sig_file2)
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 2)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        self.assertIn(golden[0], ret_list)
        self.assertIn(golden[1], ret_list)

    def test_multiple_sigsets_multiple_mapsets(self):
        # Test searching in multiple mapsets. Identical to SIG case
        rnd_sig1 = tempname(10)
        sig_file1 = f"{self.rnd_mapset_path}/signatures/sigset/{rnd_sig1}"
        f = open(sig_file1, "w")
        f.write("A sigset file")
        f.close()
        rnd_sig2 = tempname(10)
        sig_file2 = f"{self.mpath}/signatures/sigset/{rnd_sig2}"
        f = open(sig_file2, "w")
        f.write("A sigset file")
        f.close()
        self.sigfiles.append(sig_file2)
        sig_list = self.list_ptr()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIGSET, None, ctypes.byref(sig_list)
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
        # Add temporary mapset to search path and re-run test
        grass.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="add")
        # Search path is cached for this run => reset!
        G_reset_mapsets()
        ret = I_signatures_list_by_type(
            SIGFILE_TYPE_SIGSET, None, ctypes.byref(sig_list)
        )
        grass.run_command("g.mapsets", mapset=self.rnd_mapset_name, operation="remove")
        G_reset_mapsets()
        os.remove(sig_file1)
        os.remove(sig_file2)
        # There could be more sigs if this is not an empty mapset
        self.assertTrue(ret >= 2)
        ret_list = list(map(utils.decode, sig_list[:ret]))
        self.assertIn(golden[0], ret_list)
        self.assertIn(golden[1], ret_list)


if __name__ == "__main__":
    test()
