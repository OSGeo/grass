"""Test of imagery library signature management functionality

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
import os
import shutil

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import G_mapset_path, G_make_mapset
from grass.lib.imagery import (
    SIGFILE_TYPE_SIG,
    SIGFILE_TYPE_SIGSET,
    I_find_signature,
    I_signatures_remove,
    I_signatures_copy,
    I_signatures_rename,
)


class SignaturesRemoveTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sigfiles = []

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
        sigfile_name1 = "{}/signatures/sigset/{}".format(self.mpath, sig_name1)
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        sigfile_name2 = "{}/signatures/sig/{}".format(self.mpath, sig_name2)
        open(sigfile_name2, "a").close()
        self.sigfiles.append(sigfile_name2)
        sig_name3 = tempname(10)
        sigfile_name3 = "{}/signatures/sig/{}".format(self.mpath, sig_name3)
        open(sigfile_name3, "a").close()
        self.sigfiles.append(sigfile_name3)
        # Try to remove with wrong type
        ret = I_signatures_remove(SIGFILE_TYPE_SIGSET, sig_name2)
        self.assertEqual(ret, 1)
        # Try to remove with wrong mapset
        ret = I_signatures_remove(SIGFILE_TYPE_SIG, "{}@PERMANENT".format(sig_name2))
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
        sigfile_name1 = "{}/signatures/sigset/{}".format(self.mpath, sig_name1)
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        # Do not create sig_name2 matching file
        sig_name3 = tempname(10)
        sigfile_name3 = "{}/signatures/sig/{}".format(self.mpath, sig_name3)
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
        sigfile_name1 = "{}/signatures/sigset/{}".format(self.mpath, sig_name1)
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        sigfile_name2 = "{}/signatures/sigset/{}".format(self.mpath, sig_name2)
        open(sigfile_name2, "a").close()
        self.sigfiles.append(sigfile_name2)
        sig_name3 = tempname(10)
        sigfile_name3 = "{}/signatures/sig/{}".format(self.mpath, sig_name3)
        open(sigfile_name3, "a").close()
        self.sigfiles.append(sigfile_name3)
        # Try to remove with wrong type
        ret = I_signatures_remove(SIGFILE_TYPE_SIG, sig_name2)
        self.assertEqual(ret, 1)
        # Try to remove with wrong mapset
        ret = I_signatures_remove(SIGFILE_TYPE_SIGSET, "{}@PERMANENT".format(sig_name2))
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
        sigfile_name1 = "{}/signatures/sigset/{}".format(self.mpath, sig_name1)
        open(sigfile_name1, "a").close()
        self.sigfiles.append(sigfile_name1)
        sig_name2 = tempname(10)
        # Do not create sig_name2 matching file
        sig_name3 = tempname(10)
        sigfile_name3 = "{}/signatures/sig/{}".format(self.mpath, sig_name3)
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
        # A mapset with a random name
        cls.src_mapset_name = tempname(10)
        G_make_mapset(None, None, cls.src_mapset_name)
        cls.src_mapset_path = (
            cls.mpath.rsplit("/", maxsplit=1)[0] + "/" + cls.src_mapset_name
        )
        os.makedirs(cls.src_mapset_path + "/signatures/sig/")
        cls.src_sig = tempname(10)
        cls.sigfiles.append(
            "{}/signatures/sig/{}".format(cls.src_mapset_path, cls.src_sig)
        )
        f = open(cls.sigfiles[0], "w")
        f.write("A sig file")
        f.close()
        os.makedirs(cls.src_mapset_path + "/signatures/sigset/")
        cls.src_sigset = tempname(10)
        cls.sigfiles.append(
            "{}/signatures/sigset/{}".format(cls.src_mapset_path, cls.src_sigset)
        )
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
        self.sigfiles.append("{}/signatures/sig/{}".format(self.mpath, dst))
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_fq_sig(self):
        dst = tempname(10)
        self.sigfiles.append("{}/signatures/sig/{}".format(self.mpath, dst))
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
        self.sigfiles.append("{}/signatures/sigset/{}".format(self.mpath, dst))
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_fq_sigset(self):
        dst = tempname(10)
        self.sigfiles.append("{}/signatures/sigset/{}".format(self.mpath, dst))
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
        os.makedirs(cls.mpath + "/signatures/sig/", exist_ok=True)
        os.makedirs(cls.mpath + "/signatures/sigset/", exist_ok=True)

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
        sig_file = "{}/signatures/sig/{}".format(self.mpath, src_sig)
        self.sigfiles.append(sig_file)
        f = open(sig_file, "w")
        f.write("A sig file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append("{}/signatures/sig/{}".format(self.mpath, dst))
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
        sig_file = "{}/signatures/sig/{}".format(self.mpath, src_sig)
        self.sigfiles.append(sig_file)
        f = open(sig_file, "w")
        f.write("A sig file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append("{}/signatures/sig/{}".format(self.mpath, dst))
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIG, src_sig, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(
            SIGFILE_TYPE_SIG, src_sig + "@" + self.mapset_name, dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIG, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)

    def test_success_unqualified_sigset(self):
        src_sigset = tempname(10)
        sigset_file = "{}/signatures/sigset/{}".format(self.mpath, src_sigset)
        self.sigfiles.append(sigset_file)
        f = open(sigset_file, "w")
        f.write("A sigset file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append("{}/signatures/sigset/{}".format(self.mpath, dst))
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
        sigset_file = "{}/signatures/sigset/{}".format(self.mpath, src_sigset)
        self.sigfiles.append(sigset_file)
        f = open(sigset_file, "w")
        f.write("A sigset file")
        f.close()
        dst = tempname(10)
        self.sigfiles.append("{}/signatures/sigset/{}".format(self.mpath, dst))
        dst = dst + "@" + self.mapset_name
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertFalse(ret)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, src_sigset, self.mapset_name)
        self.assertTrue(ret)
        ret = I_signatures_rename(
            SIGFILE_TYPE_SIGSET, src_sigset + "@" + self.mapset_name, dst,
        )
        self.assertEqual(ret, 0)
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, dst, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)


if __name__ == "__main__":
    test()
