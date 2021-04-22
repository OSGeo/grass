"""Test of imagery library sigset file handling

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
import os
import stat
import ctypes

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import G_mapset_path
from grass.lib.raster import Rast_write_bandref
from grass.lib.imagery import (
    struct_SigSet,
    I_InitSigSet,
    I_NewClassSig,
    I_NewSubSig,
    I_WriteSigSet,
    I_ReadSigSet,
    I_fopen_sigset_file_new,
    I_fopen_sigset_file_old,
    struct_Ref,
    I_init_group_ref,
    I_add_file_to_group_ref,
    I_free_group_ref,
    String,
)


class SigSetFileTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sig_name = tempname(10)
        cls.sigfile_name = "{}/signatures/sigset/{}".format(cls.mpath, cls.sig_name)

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(cls.sigfile_name)
        except OSError:
            pass

    def test_I_fopen_signature_file_old_fail(self):
        sigfile = I_fopen_sigset_file_old(tempname(10))
        self.assertFalse(sigfile)

    def test_roundtrip_sigset_v1_one_band(self):
        """Test writing and reading back sigset file (v1)
        with a single band and fully qualified sigfile name"""

        # Create signature struct
        So = struct_SigSet()
        I_InitSigSet(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        ClassSig = I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        SubSig = I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
        self.assertEqual(So.ClassSig[0].nsubclasses, 1)

        # Fill sigset struct with data
        So.title = String("Signature title")
        So.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = String("1st class")
        So.ClassSig[0].type = 1
        So.ClassSig[0].SubSig[0].pi = 3.14
        So.ClassSig[0].SubSig[0].means[0] = 42.42
        So.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # Write signatures to file
        p_new_sigfile = I_fopen_sigset_file_new(self.sig_name)
        sig_stat = os.stat(self.sigfile_name)
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_WriteSigSet(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = struct_SigSet()
        fq_name = "{}@{}".format(self.sig_name, self.mapset_name)
        p_old_sigfile = I_fopen_sigset_file_old(fq_name)
        ret = I_ReadSigSet(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, 1)
        self.assertEqual(utils.decode(Sn.title), "Signature title")
        self.assertEqual(Sn.nbands, 1)
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
        self.assertEqual(Sn.nclasses, 1)
        self.assertEqual(Sn.ClassSig[0].nsubclasses, 1)
        self.assertEqual(Sn.ClassSig[0].used, 1)
        self.assertEqual(Sn.ClassSig[0].nsubclasses, 1)
        self.assertEqual(Sn.ClassSig[0].classnum, 2)
        self.assertEqual(utils.decode(Sn.ClassSig[0].title), "1st class")
        self.assertEqual(Sn.ClassSig[0].type, 1)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].pi, 3.14)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].means[0], 42.42)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].R[0][0], 69.69)

        # SigSet does not have free function

    def test_read_fail_sigset_v1_one_band(self):
        """Reading back should fail as band reference exceeds limit"""

        # Create signature struct
        So = struct_SigSet()
        I_InitSigSet(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        ClassSig = I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        SubSig = I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
        self.assertEqual(So.ClassSig[0].nsubclasses, 1)

        # Fill sigset struct with data
        So.title = String("Signature title")
        So.bandrefs[0] = ctypes.create_string_buffer(tempname(252).encode())
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = String("1st class")
        So.ClassSig[0].type = 1
        So.ClassSig[0].SubSig[0].pi = 3.14
        So.ClassSig[0].SubSig[0].means[0] = 42.42
        So.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # Write signatures to file
        p_new_sigfile = I_fopen_sigset_file_new(self.sig_name)
        sig_stat = os.stat(self.sigfile_name)
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_WriteSigSet(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = struct_SigSet()
        I_InitSigSet(ctypes.byref(So), 0)
        p_old_sigfile = I_fopen_sigset_file_old(self.sig_name)
        ret = I_ReadSigSet(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, -1)

        # SigSet does not have free function

    def test_roundtrip_sigset_v1_two_bands(self):
        """Test writing and reading back sigset (v1) with two bands"""

        # Create signature struct
        So = struct_SigSet()
        I_InitSigSet(ctypes.byref(So), 2)
        self.assertEqual(So.nbands, 2)
        ClassSig = I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        SubSig = I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
        self.assertEqual(So.ClassSig[0].nsubclasses, 1)

        # Fill sigset struct with data
        So.title = String("Signature title")
        So.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        So.bandrefs[1] = ctypes.create_string_buffer(b"The_Who")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = String("1st class")
        So.ClassSig[0].type = 1
        So.ClassSig[0].SubSig[0].pi = 3.14
        So.ClassSig[0].SubSig[0].means[0] = 42.42
        So.ClassSig[0].SubSig[0].means[1] = 24.24
        So.ClassSig[0].SubSig[0].R[0][0] = 69.69
        So.ClassSig[0].SubSig[0].R[0][1] = 13.37
        So.ClassSig[0].SubSig[0].R[1][0] = 13.37
        So.ClassSig[0].SubSig[0].R[1][1] = 21.21

        # Write signatures to file
        p_new_sigfile = I_fopen_sigset_file_new(self.sig_name)
        sig_stat = os.stat(self.sigfile_name)
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_WriteSigSet(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = struct_SigSet()
        p_old_sigfile = I_fopen_sigset_file_old(self.sig_name)
        ret = I_ReadSigSet(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, 1)
        self.assertEqual(utils.decode(Sn.title), "Signature title")
        self.assertEqual(Sn.nbands, 2)
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Who")
        self.assertEqual(Sn.nclasses, 1)
        self.assertEqual(Sn.ClassSig[0].nsubclasses, 1)
        self.assertEqual(Sn.ClassSig[0].used, 1)
        self.assertEqual(Sn.ClassSig[0].nsubclasses, 1)
        self.assertEqual(Sn.ClassSig[0].classnum, 2)
        self.assertEqual(utils.decode(Sn.ClassSig[0].title), "1st class")
        self.assertEqual(Sn.ClassSig[0].type, 1)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].pi, 3.14)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].means[0], 42.42)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].means[1], 24.24)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].R[0][0], 69.69)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].R[0][1], 13.37)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].R[1][0], 13.37)
        self.assertEqual(Sn.ClassSig[0].SubSig[0].R[1][1], 21.21)

        # SigSet does not have free function


if __name__ == "__main__":
    test()
