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
    SigSet,
    I_InitSigSet,
    I_NewClassSig,
    I_NewSubSig,
    I_WriteSigSet,
    I_ReadSigSet,
    I_SortSigSetByBandref,
    I_fopen_sigset_file_new,
    I_fopen_sigset_file_old,
    Ref,
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
        So = SigSet()
        I_InitSigSet(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
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
        Sn = SigSet()
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
        So = SigSet()
        I_InitSigSet(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
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
        Sn = SigSet()
        I_InitSigSet(ctypes.byref(So), 0)
        p_old_sigfile = I_fopen_sigset_file_old(self.sig_name)
        ret = I_ReadSigSet(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, -1)

        # SigSet does not have free function

    def test_roundtrip_sigset_v1_two_bands(self):
        """Test writing and reading back sigset (v1) with two bands"""

        # Create signature struct
        So = SigSet()
        I_InitSigSet(ctypes.byref(So), 2)
        self.assertEqual(So.nbands, 2)
        I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
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
        Sn = SigSet()
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


class SortSigSetByBandrefTest(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mapset = Mapset().name
        cls.map1 = tempname(10)
        cls.bandref1 = "The_Doors"
        cls.map2 = tempname(10)
        cls.bandref2 = "The_Who"
        cls.map3 = tempname(10)
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map1))
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map2))
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map3))
        Rast_write_bandref(cls.map1, cls.bandref1)
        Rast_write_bandref(cls.map2, cls.bandref2)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map1)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map2)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map3)

    def test_symmetric_complete_difference(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)

        # Prepare sigset struct
        S = SigSet()
        I_InitSigSet(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        I_NewClassSig(ctypes.byref(S))
        self.assertEqual(S.nclasses, 1)
        I_NewSubSig(ctypes.byref(S), ctypes.byref(S.ClassSig[0]))
        self.assertEqual(S.ClassSig[0].nsubclasses, 1)
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in two error strings in ret
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(sig_err, "The_Troggs")
        self.assertEqual(ref_err, "The_Doors")

        # Clean up memory to help track memory leaks when run by valgrind
        # I_free_sigset is missing
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)

    def test_asymmetric_complete_difference(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = SigSet()
        I_InitSigSet(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        I_NewClassSig(ctypes.byref(S))
        self.assertEqual(S.nclasses, 1)
        I_NewSubSig(ctypes.byref(S), ctypes.byref(S.ClassSig[0]))
        self.assertEqual(S.ClassSig[0].nsubclasses, 1)
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in two error strings in ret
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(sig_err, "The_Troggs")
        self.assertEqual(ref_err, "The_Doors,The_Who")

        # Clean up memory to help track memory leaks when run by valgrind
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)

    def test_missing_bandref(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)
        ret = I_add_file_to_group_ref(self.map3, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 2)

        # Prepare signature struct
        S = SigSet()
        I_InitSigSet(ctypes.byref(S), 10)
        self.assertEqual(S.nbands, 10)
        I_NewClassSig(ctypes.byref(S))
        self.assertEqual(S.nclasses, 1)
        I_NewSubSig(ctypes.byref(S), ctypes.byref(S.ClassSig[0]))
        self.assertEqual(S.ClassSig[0].nsubclasses, 1)
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in two error strings in ret
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(
            sig_err,
            "<band reference missing>,<band reference missing>,"
            + "<band reference missing>,<band reference missing>,"
            + "<band reference missing>,<band reference missing>,"
            + "<band reference missing>,<band reference missing>,"
            + "<band reference missing>",
        )
        self.assertEqual(ref_err, "The_Doors,<band reference missing>")

        # Clean up memory to help track memory leaks when run by valgrind
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)

    def test_single_complete_match(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)

        # Prepare signature struct
        S = SigSet()
        I_InitSigSet(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        I_NewClassSig(ctypes.byref(S))
        self.assertEqual(S.nclasses, 1)
        I_NewSubSig(ctypes.byref(S), ctypes.byref(S.ClassSig[0]))
        self.assertEqual(S.ClassSig[0].nsubclasses, 1)
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in returning NULL
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        bandref = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
        self.assertEqual(S.ClassSig[0].SubSig[0].pi, 3.14)
        self.assertEqual(S.ClassSig[0].SubSig[0].means[0], 42.42)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[0][0], 69.69)

        # Clean up memory to help track memory leaks when run by valgrind
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)

    def test_double_complete_match_reorder(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = SigSet()
        I_InitSigSet(ctypes.byref(S), 2)
        self.assertEqual(S.nbands, 2)
        I_NewClassSig(ctypes.byref(S))
        self.assertEqual(S.nclasses, 1)
        I_NewSubSig(ctypes.byref(S), ctypes.byref(S.ClassSig[0]))
        self.assertEqual(S.ClassSig[0].nsubclasses, 1)
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.bandrefs[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].means[1] = 24.24
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69
        S.ClassSig[0].SubSig[0].R[0][1] = 96.96
        S.ClassSig[0].SubSig[0].R[1][0] = -69.69
        S.ClassSig[0].SubSig[0].R[1][1] = -96.96

        # This should result in returning NULL
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # Band references and sig items should be swapped
        # Static items
        self.assertEqual(S.ClassSig[0].SubSig[0].pi, 3.14)
        # Reordered items
        bandref1 = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref1, "The_Doors")
        bandref2 = utils.decode(ctypes.cast(S.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref2, "The_Who")
        self.assertEqual(S.ClassSig[0].SubSig[0].means[0], 24.24)
        self.assertEqual(S.ClassSig[0].SubSig[0].means[1], 42.42)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[0][0], -96.96)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[0][1], -69.69)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[1][0], 96.96)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[1][1], 69.69)

        # Clean up memory to help track memory leaks when run by valgrind
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)

    def test_double_complete_match_same_order(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = SigSet()
        I_InitSigSet(ctypes.byref(S), 2)
        self.assertEqual(S.nbands, 2)
        I_NewClassSig(ctypes.byref(S))
        self.assertEqual(S.nclasses, 1)
        I_NewSubSig(ctypes.byref(S), ctypes.byref(S.ClassSig[0]))
        self.assertEqual(S.ClassSig[0].nsubclasses, 1)
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.bandrefs[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].means[1] = 24.24
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69
        S.ClassSig[0].SubSig[0].R[0][1] = 96.96
        S.ClassSig[0].SubSig[0].R[1][0] = -69.69
        S.ClassSig[0].SubSig[0].R[1][1] = -96.96

        # This should result in returning NULL
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # Band references and sig items should not be swapped
        # Static items
        self.assertEqual(S.ClassSig[0].SubSig[0].pi, 3.14)
        # Reordered items
        bandref1 = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref1, "The_Who")
        bandref2 = utils.decode(ctypes.cast(S.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref2, "The_Doors")
        self.assertEqual(S.ClassSig[0].SubSig[0].means[0], 42.42)
        self.assertEqual(S.ClassSig[0].SubSig[0].means[1], 24.24)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[0][0], 69.69)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[0][1], 96.96)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[1][0], -69.69)
        self.assertEqual(S.ClassSig[0].SubSig[0].R[1][1], -96.96)

        # Clean up memory to help track memory leaks when run by valgrind
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)


if __name__ == "__main__":
    test()
