"""Test of imagery library file searching functionality

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
    I_find_signature2,
)


class FindSignatureTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sig_name1 = tempname(10)
        cls.sigfile_name1 = "{}/signatures/sigset/{}".format(cls.mpath, cls.sig_name1)
        open(cls.sigfile_name1, "a").close()
        cls.sig_name2 = tempname(10)
        cls.sigfile_name2 = "{}/signatures/sig/{}".format(cls.mpath, cls.sig_name2)
        open(cls.sigfile_name2, "a").close()

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(cls.sigfile_name1)
            os.remove(cls.sigfile_name2)
        except OSError:
            pass

    def test_find_sig(self):
        # Non existing without a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIG, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIG, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sigset with sig type should equal non existing
        ret = I_find_signature(SIGFILE_TYPE_SIG, self.sig_name1, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIG, self.sig_name2, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIG, self.sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature(SIGFILE_TYPE_SIG, self.sig_name2, "PERMANENT")
        self.assertFalse(ret)

    def test_find_sigset(self):
        # Non existing without a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sig with sigset type should equal non existing
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, self.sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, self.sig_name1, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, self.sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature(SIGFILE_TYPE_SIGSET, self.sig_name1, "PERMANENT")
        self.assertFalse(ret)

    def test_find2_sig(self):
        # Non existing without a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIG, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIG, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sigset with sig type should equal non existing
        ret = I_find_signature2(SIGFILE_TYPE_SIG, self.sig_name1, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIG, self.sig_name2, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIG, self.sig_name2, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature2(SIGFILE_TYPE_SIG, self.sig_name2, "PERMANENT")
        self.assertFalse(ret)

    def test_find2_sigset(self):
        # Non existing without a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIGSET, tempname(10), None)
        self.assertFalse(ret)
        # Non existing with a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIGSET, tempname(10), self.mapset_name)
        self.assertFalse(ret)
        # Sig with sigset type should equal non existing
        ret = I_find_signature2(SIGFILE_TYPE_SIGSET, self.sig_name2, self.mapset_name)
        self.assertFalse(ret)
        # Existing without a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIGSET, self.sig_name1, None)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing with a mapset
        ret = I_find_signature2(SIGFILE_TYPE_SIGSET, self.sig_name1, self.mapset_name)
        self.assertTrue(ret)
        ms = utils.decode(ret)
        self.assertEqual(ms, self.mapset_name)
        # Existing in a different mapset should fail
        ret = I_find_signature2(SIGFILE_TYPE_SIGSET, self.sig_name1, "PERMANENT")
        self.assertFalse(ret)


if __name__ == "__main__":
    test()
