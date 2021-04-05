"""Test of imagery library signature file handling

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
import os
import stat
import random
import string
import ctypes

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass.gis import Mapset
from grass.pygrass import utils

from grass.lib.gis import G_remove_misc, G_mapset_path
from grass.lib.raster import Rast_legal_bandref, Rast_read_bandref, Rast_write_bandref
from grass.lib.imagery import struct_Signature, I_init_signatures, I_fopen_signature_file_new, I_new_signature, I_write_signatures, I_fopen_signature_file_old, I_read_signatures
from grass.pygrass.gis import Gisdbase, Location, Mapset


class SignatureFileTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mpath = utils.decode(G_mapset_path())
        cls.sig_name = tempname(10)
        cls.sigfile_name = "{}/signatures/sig/{}".format(cls.mpath, cls.sig_name)

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(cls.sigfile_name)
        except OSError:
            pass

    def test_I_fopen_signature_file_old_fail(self):
        sigfile = I_fopen_signature_file_old(tempname(10))
        self.assertFalse(sigfile)

    def test_roundtrip_signature_v1_norgb(self):
        """Test writing and reading back signature file (v1)"""

        # Create signature struct
        So = struct_Signature()
        I_init_signatures(ctypes.byref(So), 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertTrue(sig_count == 1)
        # char **
        LP_c_char = ctypes.POINTER(ctypes.c_char)
        LP_LP_c_char = ctypes.POINTER(LP_c_char)
        So.bandrefs = ctypes.cast((LP_LP_c_char * 1)(), LP_LP_c_char)

        # Fill signatures struct with data
        So.title = b"Signature title"
        So.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        So.sig[0].status = 1
        So.sig[0].have_color = 0
        So.sig[0].npoints = 42
        So.sig[0].desc = b"my label"
        So.sig[0].mean[0] = 2.5
        So.sig[0].var[0][0] = 0.7

        # Write signatures to file
        p_new_sigfile = I_fopen_signature_file_new(self.sig_name)
        sig_stat = os.stat(self.sigfile_name)
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_write_signatures(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = struct_Signature()
        I_init_signatures(ctypes.byref(Sn), 0)
        p_old_sigfile = I_fopen_signature_file_old(self.sig_name)
        ret = I_read_signatures(p_old_sigfile, ctypes.byref(Sn))
        self.assertTrue(ret == 1)
        self.assertTrue(Sn.title == b"Signature title")
        self.assertTrue(Sn.nbands == 1)
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[0], ctypes.c_char_p).value)
        self.assertTrue(bandref == "The_Doors")
        self.assertTrue(Sn.sig[0].status == 1)
        self.assertTrue(Sn.sig[0].have_color == 0)
        self.assertTrue(Sn.sig[0].npoints == 42)
        self.assertTrue(Sn.sig[0].desc == b"my label")
        self.assertTrue(Sn.sig[0].mean[0] == 2.5)
        self.assertTrue(Sn.sig[0].var[0][0] == 0.7)


if __name__ == "__main__":
    test()
