"""Test of imagery library signature management functionality

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import G_mapset_path
from grass.lib.imagery import (
    SIGFILE_TYPE_SIG,
    SIGFILE_TYPE_SIGSET,
    I_find_signature,
    I_signatures_remove,
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


if __name__ == "__main__":
    test()
