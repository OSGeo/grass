"""Test of imagery library signature file handling

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
    struct_Signature,
    struct_Ref,
    I_init_signatures,
    I_new_signature,
    I_fopen_signature_file_new,
    I_write_signatures,
    I_fopen_signature_file_old,
    I_read_signatures,
    I_sort_signatures_by_bandref,
    I_free_signatures,
    I_init_group_ref,
    I_add_file_to_group_ref,
    I_free_group_ref,
)


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

    def test_roundtrip_signature_v1_norgb_one_band(self):
        """Test writing and reading back signature file (v1)
        wiht a single band"""

        # Create signature struct
        So = struct_Signature()
        I_init_signatures(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 1)

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
        self.assertEqual(ret, 1)
        self.assertEqual(Sn.title, b"Signature title")
        self.assertEqual(Sn.nbands, 1)
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
        self.assertEqual(Sn.sig[0].status, 1)
        self.assertEqual(Sn.sig[0].have_color, 0)
        self.assertEqual(Sn.sig[0].npoints, 42)
        self.assertEqual(Sn.sig[0].desc, b"my label")
        self.assertEqual(Sn.sig[0].mean[0], 2.5)
        self.assertEqual(Sn.sig[0].var[0][0], 0.7)

        # Free signature struct after use
        I_free_signatures(ctypes.byref(Sn))
        self.assertEqual(Sn.nbands, 0)
        self.assertEqual(Sn.nsigs, 0)

    def test_broken_signature_v1_norgb(self):
        """Test reading back signature file (v1) should fail due to
        single band reference exceeding maximum length"""

        # Create signature struct
        So = struct_Signature()
        I_init_signatures(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 1)

        # Fill signatures struct with data
        So.title = b"Signature title"
        # len(tempname(251)) == 255
        So.bandrefs[0] = ctypes.create_string_buffer(tempname(251).encode())
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
        self.assertEqual(ret, -1)

    def test_roundtrip_signature_v1_norgb_two_bands(self):
        """Test writing and reading back signature (v1) with two bands"""

        # Create signature struct
        So = struct_Signature()
        I_init_signatures(ctypes.byref(So), 2)
        self.assertEqual(So.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 2)

        # Fill signatures struct with data
        So.title = b"Signature title"
        So.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        So.bandrefs[1] = ctypes.create_string_buffer(b"The_Who")
        So.sig[0].status = 1
        So.sig[0].have_color = 0
        So.sig[0].npoints = 42
        So.sig[0].desc = b"my label1"
        So.sig[0].mean[0] = 2.5
        So.sig[0].mean[1] = 3.5
        So.sig[0].var[0][0] = 0.7
        So.sig[0].var[1][0] = 0.2
        So.sig[0].var[1][1] = 0.8
        So.sig[1].status = 1
        So.sig[1].have_color = 0
        So.sig[1].npoints = 69
        So.sig[1].desc = b"my label2"
        So.sig[1].mean[0] = 3.5
        So.sig[1].mean[1] = 4.5
        So.sig[1].var[0][0] = 1.7
        So.sig[1].var[1][0] = 1.2
        So.sig[1].var[1][1] = 1.8

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
        self.assertEqual(ret, 1)
        self.assertEqual(Sn.title, b"Signature title")
        self.assertEqual(Sn.nbands, 2)
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
        self.assertEqual(Sn.sig[0].status, 1)
        self.assertEqual(Sn.sig[0].have_color, 0)
        self.assertEqual(Sn.sig[0].npoints, 42)
        self.assertEqual(Sn.sig[0].desc, b"my label1")
        self.assertEqual(Sn.sig[0].mean[0], 2.5)
        self.assertEqual(Sn.sig[0].mean[1], 3.5)
        self.assertEqual(Sn.sig[0].var[0][0], 0.7)
        self.assertEqual(Sn.sig[0].var[1][0], 0.2)
        self.assertEqual(Sn.sig[0].var[1][1], 0.8)
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Who")
        self.assertEqual(Sn.sig[1].status, 1)
        self.assertEqual(Sn.sig[1].have_color, 0)
        self.assertEqual(Sn.sig[1].npoints, 69)
        self.assertEqual(Sn.sig[1].desc, b"my label2")
        self.assertEqual(Sn.sig[1].mean[0], 3.5)
        self.assertEqual(Sn.sig[1].mean[1], 4.5)
        self.assertEqual(Sn.sig[1].var[0][0], 1.7)
        self.assertEqual(Sn.sig[1].var[1][0], 1.2)
        self.assertEqual(Sn.sig[1].var[1][1], 1.8)

        # Free signature struct after use
        I_free_signatures(ctypes.byref(Sn))
        self.assertEqual(Sn.nbands, 0)
        self.assertEqual(Sn.nsigs, 0)


class SortSignaturesByBandrefTest(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mapset = Mapset().name
        cls.map1 = tempname(10)
        cls.bandref1 = "The_Doors"
        cls.map2 = tempname(10)
        cls.bandref2 = "The_Who"
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map1))
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map2))
        Rast_write_bandref(cls.map1, cls.bandref1)
        Rast_write_bandref(cls.map2, cls.bandref2)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map1)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map2)

    def test_symmetric_complete_difference(self):
        # Prepare imagery group reference struct
        R = struct_Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)

        # Prepare signature struct
        S = struct_Signature()
        I_init_signatures(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.title = b"Signature title"
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in two error strings in ret
        ret = I_sort_signatures_by_bandref(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(sig_err, "The_Troggs")
        self.assertEqual(ref_err, "The_Doors")

        # Clean up memory to help track memory leaks when run by valgrind
        S.bandrefs[0] = None  # C should not call free() on memory allocated by python
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret[0]:
            self.libc.free(ret[0])
        if ret[1]:
            self.libc.free(ret[1])
        self.libc.free(ret)

    def test_asymmetric_complete_difference(self):
        # Prepare imagery group reference struct
        R = struct_Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = struct_Signature()
        I_init_signatures(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.title = b"Signature title"
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in two error strings in ret
        ret = I_sort_signatures_by_bandref(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(sig_err, "The_Troggs")
        self.assertEqual(ref_err, "The_Doors,The_Who")

        # Clean up memory to help track memory leaks when run by valgrind
        S.bandrefs[0] = None
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret[0]:
            self.libc.free(ret[0])
        if ret[1]:
            self.libc.free(ret[1])
        self.libc.free(ret)

    def test_missing_bandref(self):
        # Prepare imagery group reference struct
        R = struct_Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = struct_Signature()
        I_init_signatures(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.title = b"Signature title"
        # S.bandrefs[0] = missing value
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in two error strings in ret
        ret = I_sort_signatures_by_bandref(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertFalse(sig_err)
        self.assertEqual(ref_err, "The_Doors,The_Who")

        # Clean up memory to help track memory leaks when run by valgrind
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret[0]:
            self.libc.free(ret[0])
        if ret[1]:
            self.libc.free(ret[1])
        self.libc.free(ret)

    def test_single_complete_match(self):
        # Prepare imagery group reference struct
        R = struct_Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)

        # Prepare signature struct
        S = struct_Signature()
        I_init_signatures(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.title = b"Signature title"
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in returning NULL
        ret = I_sort_signatures_by_bandref(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        bandref = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
        self.assertEqual(S.sig[0].mean[0], 2.5)
        self.assertEqual(S.sig[0].var[0][0], 0.7)

        # Clean up memory to help track memory leaks when run by valgrind
        S.bandrefs[0] = None
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret[0]:
            self.libc.free(ret[0])
        if ret[1]:
            self.libc.free(ret[1])
        self.libc.free(ret)

    def test_double_complete_match_reorder(self):
        # Prepare imagery group reference struct
        R = struct_Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = struct_Signature()
        I_init_signatures(ctypes.byref(S), 2)
        self.assertEqual(S.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 2)
        S.title = b"Signature title"
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.bandrefs[1] = ctypes.create_string_buffer(b"The_Doors")
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 69
        S.sig[0].desc = b"my label2"
        S.sig[0].mean[0] = 3.3
        S.sig[0].mean[1] = 6.6
        S.sig[0].var[0][0] = 1.7
        S.sig[0].var[1][0] = 1.2
        S.sig[0].var[1][1] = 1.8
        S.sig[1].status = 1
        S.sig[1].have_color = 0
        S.sig[1].npoints = 42
        S.sig[1].desc = b"my label1"
        S.sig[1].mean[0] = 2.2
        S.sig[1].mean[1] = 4.4
        S.sig[1].var[0][0] = 0.7
        S.sig[1].var[1][0] = 0.2
        S.sig[1].var[1][1] = 0.8

        # This should result in returning NULL
        ret = I_sort_signatures_by_bandref(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # Band references and sig items should be swapped
        # Static items
        self.assertEqual(S.sig[0].npoints, 69)
        self.assertEqual(S.sig[1].npoints, 42)
        # Reordered items
        bandref1 = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref1, "The_Doors")
        bandref2 = utils.decode(ctypes.cast(S.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref2, "The_Who")
        self.assertEqual(S.sig[0].mean[0], 6.6)
        self.assertEqual(S.sig[0].mean[1], 3.3)
        self.assertEqual(S.sig[0].var[0][0], 1.8)
        self.assertEqual(S.sig[0].var[1][0], 1.2)
        self.assertEqual(S.sig[0].var[1][1], 1.7)
        self.assertEqual(S.sig[1].mean[0], 4.4)
        self.assertEqual(S.sig[1].mean[1], 2.2)
        self.assertEqual(S.sig[1].var[0][0], 0.8)
        self.assertEqual(S.sig[1].var[1][0], 0.2)
        self.assertEqual(S.sig[1].var[1][1], 0.7)

        # Clean up memory to help track memory leaks when run by valgrind
        S.bandrefs[0] = None
        S.bandrefs[1] = None
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret[0]:
            self.libc.free(ret[0])
        if ret[1]:
            self.libc.free(ret[1])
        self.libc.free(ret)

    def test_double_complete_match_same_order(self):
        # Prepare imagery group reference struct
        R = struct_Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = struct_Signature()
        I_init_signatures(ctypes.byref(S), 2)
        self.assertEqual(S.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 2)
        S.title = b"Signature title"
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.bandrefs[1] = ctypes.create_string_buffer(b"The_Doors")
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 69
        S.sig[0].desc = b"my label2"
        S.sig[0].mean[0] = 3.5
        S.sig[0].var[0][0] = 1.7
        S.sig[0].var[1][0] = 1.2
        S.sig[0].var[1][1] = 1.8
        S.sig[1].status = 1
        S.sig[1].have_color = 0
        S.sig[1].npoints = 42
        S.sig[1].desc = b"my label1"
        S.sig[1].mean[0] = 2.5
        S.sig[1].var[0][0] = 0.7
        S.sig[1].var[1][0] = 0.2
        S.sig[1].var[1][1] = 0.8

        # This should result in returning NULL
        ret = I_sort_signatures_by_bandref(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # Band references and sig items should not be swapped
        # Static items
        self.assertEqual(S.sig[0].npoints, 69)
        self.assertEqual(S.sig[1].npoints, 42)
        # Reordered items
        bandref1 = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref1, "The_Who")
        bandref2 = utils.decode(ctypes.cast(S.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref2, "The_Doors")
        self.assertEqual(S.sig[0].mean[0], 3.5)
        self.assertEqual(S.sig[0].var[0][0], 1.7)
        self.assertEqual(S.sig[1].mean[0], 2.5)
        self.assertEqual(S.sig[1].var[0][0], 0.7)

        # Clean up memory to help track memory leaks when run by valgrind
        S.bandrefs[0] = None
        S.bandrefs[1] = None
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret[0]:
            self.libc.free(ret[0])
        if ret[1]:
            self.libc.free(ret[1])
        self.libc.free(ret)


if __name__ == "__main__":
    test()
