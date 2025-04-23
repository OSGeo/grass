"""Test of imagery library signature file handling

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""

import stat
import ctypes
import os
import shutil
from pathlib import Path

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import G_mapset_path
from grass.lib.raster import Rast_write_semantic_label
from grass.lib.imagery import (
    Signature,
    Ref,
    I_init_signatures,
    I_new_signature,
    I_fopen_signature_file_new,
    I_write_signatures,
    I_fopen_signature_file_old,
    I_read_signatures,
    I_sort_signatures_by_semantic_label,
    I_free_signatures,
    I_init_group_ref,
    I_add_file_to_group_ref,
    I_free_group_ref,
)


class SignatureFileTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        if os.name == "nt":
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("msvcrt"))
        else:
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sig_name = tempname(10)
        cls.sig_dir = f"{cls.mpath}/signatures/sig/{cls.sig_name}"

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.sig_dir, ignore_errors=True)

    def test_I_fopen_signature_file_old_fail(self):
        sigfile = I_fopen_signature_file_old(tempname(10))
        self.assertFalse(sigfile)

    def test_roundtrip_signature_v1_norgb_one_label(self):
        """Test writing and reading back signature file (v1)
        with a single label"""

        # Create signature struct
        So = Signature()
        I_init_signatures(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 1)

        # Fill signatures struct with data
        So.title = b"Signature title"
        So.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        So.sig[0].status = 1
        So.sig[0].have_color = 0
        So.sig[0].npoints = 42
        So.sig[0].desc = b"my label"
        So.sig[0].mean[0] = 2.5
        So.sig[0].var[0][0] = 0.7

        # Write signatures to file
        p_new_sigfile = I_fopen_signature_file_new(self.sig_name)
        sig_stat = Path(self.sig_dir, "sig").stat()
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_write_signatures(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = Signature()
        fq_name = f"{self.sig_name}@{self.mapset_name}"
        p_old_sigfile = I_fopen_signature_file_old(fq_name)
        ret = I_read_signatures(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, 1)
        self.assertEqual(Sn.title, b"Signature title")
        self.assertEqual(Sn.nbands, 1)
        self.assertEqual(Sn.have_oclass, 0)
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
        self.assertEqual(Sn.sig[0].status, 1)
        self.assertEqual(Sn.sig[0].have_color, 0)
        self.assertEqual(Sn.sig[0].npoints, 42)
        self.assertEqual(Sn.sig[0].desc, b"my label")
        self.assertEqual(Sn.sig[0].mean[0], 2.5)
        self.assertEqual(Sn.sig[0].var[0][0], 0.7)

        # Free signature struct after use
        So.semantic_labels[0] = None
        I_free_signatures(ctypes.byref(So))
        I_free_signatures(ctypes.byref(Sn))
        self.assertEqual(Sn.nbands, 0)
        self.assertEqual(Sn.nsigs, 0)

    def test_broken_signature_v1_norgb(self):
        """Test reading back signature file (v1) should fail due to
        single semantic label exceeding maximum length"""

        # Create signature struct
        So = Signature()
        I_init_signatures(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 1)

        # Fill signatures struct with data
        So.title = b"Signature title"
        # len(tempname(251)) == 255
        So.semantic_labels[0] = ctypes.create_string_buffer(tempname(251).encode())
        So.sig[0].status = 1
        So.sig[0].have_color = 0
        So.sig[0].npoints = 42
        So.sig[0].desc = b"my label"
        So.sig[0].mean[0] = 2.5
        So.sig[0].var[0][0] = 0.7

        # Write signatures to file
        p_new_sigfile = I_fopen_signature_file_new(self.sig_name)
        sig_stat = Path(self.sig_dir, "sig").stat()
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_write_signatures(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = Signature()
        p_old_sigfile = I_fopen_signature_file_old(self.sig_name)
        ret = I_read_signatures(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, -1)

        So.semantic_labels[0] = None
        I_free_signatures(ctypes.byref(So))
        I_free_signatures(ctypes.byref(Sn))

    def test_roundtrip_signature_v1_norgb_two_labelss(self):
        """Test writing and reading back signature (v1) with two labels"""

        # Create signature struct
        So = Signature()
        I_init_signatures(ctypes.byref(So), 2)
        self.assertEqual(So.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 2)

        # Fill signatures struct with data
        So.title = b"Signature title"
        So.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        So.semantic_labels[1] = ctypes.create_string_buffer(b"The_Who")
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
        sig_stat = Path(self.sig_dir, "sig").stat()
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_write_signatures(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = Signature()
        p_old_sigfile = I_fopen_signature_file_old(self.sig_name)
        ret = I_read_signatures(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, 1)
        self.assertEqual(Sn.title, b"Signature title")
        self.assertEqual(Sn.nbands, 2)
        self.assertEqual(Sn.have_oclass, 0)
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
        self.assertEqual(Sn.sig[0].status, 1)
        self.assertEqual(Sn.sig[0].have_color, 0)
        self.assertEqual(Sn.sig[0].npoints, 42)
        self.assertEqual(Sn.sig[0].desc, b"my label1")
        self.assertEqual(Sn.sig[0].mean[0], 2.5)
        self.assertEqual(Sn.sig[0].mean[1], 3.5)
        self.assertEqual(Sn.sig[0].var[0][0], 0.7)
        self.assertEqual(Sn.sig[0].var[1][0], 0.2)
        self.assertEqual(Sn.sig[0].var[1][1], 0.8)
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Who")
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
        So.semantic_labels[0] = None
        So.semantic_labels[1] = None
        I_free_signatures(ctypes.byref(So))
        I_free_signatures(ctypes.byref(Sn))
        self.assertEqual(Sn.nbands, 0)
        self.assertEqual(Sn.nsigs, 0)

    def test_roundtrip_signature_v2_norgb_two_labels_oclass(self):
        """Test writing and reading back signature (v1) with two labels
        and original class values"""

        # Create signature struct
        So = Signature()
        I_init_signatures(ctypes.byref(So), 2)
        self.assertEqual(So.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(So))
        self.assertEqual(sig_count, 2)

        # Fill signatures struct with data
        So.title = b"Signature title"
        So.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        So.semantic_labels[1] = ctypes.create_string_buffer(b"The_Who")
        So.have_oclass = 1
        So.sig[0].status = 1
        So.sig[0].have_color = 0
        So.sig[0].oclass = 1337
        So.sig[0].npoints = 42
        So.sig[0].desc = b"my label1"
        So.sig[0].mean[0] = 2.5
        So.sig[0].mean[1] = 3.5
        So.sig[0].var[0][0] = 0.7
        So.sig[0].var[1][0] = 0.2
        So.sig[0].var[1][1] = 0.8
        So.sig[1].status = 1
        So.sig[1].have_color = 0
        So.sig[1].oclass = 59009
        So.sig[1].npoints = 69
        So.sig[1].desc = b"my label2"
        So.sig[1].mean[0] = 3.5
        So.sig[1].mean[1] = 4.5
        So.sig[1].var[0][0] = 1.7
        So.sig[1].var[1][0] = 1.2
        So.sig[1].var[1][1] = 1.8

        # Write signatures to file
        p_new_sigfile = I_fopen_signature_file_new(self.sig_name)
        sig_stat = Path(self.sig_dir, "sig").stat()
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_write_signatures(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = Signature()
        p_old_sigfile = I_fopen_signature_file_old(self.sig_name)
        ret = I_read_signatures(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, 1)
        self.assertEqual(Sn.title, b"Signature title")
        self.assertEqual(Sn.nbands, 2)
        self.assertEqual(Sn.have_oclass, 1)
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
        self.assertEqual(Sn.sig[0].status, 1)
        self.assertEqual(Sn.sig[0].have_color, 0)
        self.assertEqual(Sn.sig[0].npoints, 42)
        self.assertEqual(Sn.sig[0].oclass, 1337)
        self.assertEqual(Sn.sig[0].desc, b"my label1")
        self.assertEqual(Sn.sig[0].mean[0], 2.5)
        self.assertEqual(Sn.sig[0].mean[1], 3.5)
        self.assertEqual(Sn.sig[0].var[0][0], 0.7)
        self.assertEqual(Sn.sig[0].var[1][0], 0.2)
        self.assertEqual(Sn.sig[0].var[1][1], 0.8)
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Who")
        self.assertEqual(Sn.sig[1].status, 1)
        self.assertEqual(Sn.sig[1].have_color, 0)
        self.assertEqual(Sn.sig[1].npoints, 69)
        self.assertEqual(Sn.sig[1].oclass, 59009)
        self.assertEqual(Sn.sig[1].desc, b"my label2")
        self.assertEqual(Sn.sig[1].mean[0], 3.5)
        self.assertEqual(Sn.sig[1].mean[1], 4.5)
        self.assertEqual(Sn.sig[1].var[0][0], 1.7)
        self.assertEqual(Sn.sig[1].var[1][0], 1.2)
        self.assertEqual(Sn.sig[1].var[1][1], 1.8)

        # Free signature struct after use
        So.semantic_labels[0] = None
        So.semantic_labels[1] = None
        I_free_signatures(ctypes.byref(So))
        I_free_signatures(ctypes.byref(Sn))
        self.assertEqual(Sn.nbands, 0)
        self.assertEqual(Sn.nsigs, 0)


class SortSignaturesBysemantic_labelTest(TestCase):
    @classmethod
    def setUpClass(cls):
        if os.name == "nt":
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("msvcrt"))
        else:
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mapset = Mapset().name
        cls.map1 = tempname(10)
        cls.semantic_label1 = "The_Doors"
        cls.map2 = tempname(10)
        cls.semantic_label2 = "The_Who"
        cls.map3 = tempname(10)
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression=f"{cls.map1} = 1")
        cls.runModule("r.mapcalc", expression=f"{cls.map2} = 1")
        cls.runModule("r.mapcalc", expression=f"{cls.map3} = 1")
        Rast_write_semantic_label(cls.map1, cls.semantic_label1)
        Rast_write_semantic_label(cls.map2, cls.semantic_label2)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(cls.map1, cls.map2, cls.map3),
        )

    def test_symmetric_complete_difference(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)

        # Prepare signature struct
        S = Signature()
        I_init_signatures(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.title = b"Signature title"
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in two error strings in ret
        ret = I_sort_signatures_by_semantic_label(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(sig_err, "The_Troggs")
        self.assertEqual(ref_err, "The_Doors")

        # Clean up memory to help track memory leaks when run by valgrind
        S.semantic_labels[0] = (
            None  # C should not call free() on memory allocated by python
        )
        I_free_signatures(ctypes.byref(S))
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
        S = Signature()
        I_init_signatures(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.title = b"Signature title"
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in two error strings in ret
        ret = I_sort_signatures_by_semantic_label(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(sig_err, "The_Troggs")
        self.assertEqual(ref_err, "The_Doors,The_Who")

        # Clean up memory to help track memory leaks when run by valgrind
        S.semantic_labels[0] = None
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)

    def test_missing_semantic_label(self):
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
        S = Signature()
        I_init_signatures(ctypes.byref(S), 10)
        self.assertEqual(S.nbands, 10)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.title = b"Signature title"
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in two error strings in ret
        ret = I_sort_signatures_by_semantic_label(ctypes.byref(S), ctypes.byref(R))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(
            sig_err,
            "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>",
        )
        self.assertEqual(ref_err, f"The_Doors,{self.map3}")

        # Clean up memory to help track memory leaks when run by valgrind
        S.semantic_labels[0] = None
        I_free_signatures(ctypes.byref(S))
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
        S = Signature()
        I_init_signatures(ctypes.byref(S), 1)
        self.assertEqual(S.nbands, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        S.title = b"Signature title"
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        S.sig[0].status = 1
        S.sig[0].have_color = 0
        S.sig[0].npoints = 42
        S.sig[0].desc = b"my label"
        S.sig[0].mean[0] = 2.5
        S.sig[0].var[0][0] = 0.7

        # This should result in returning NULL
        ret = I_sort_signatures_by_semantic_label(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        semantic_label = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
        self.assertEqual(S.sig[0].mean[0], 2.5)
        self.assertEqual(S.sig[0].var[0][0], 0.7)

        # Clean up memory to help track memory leaks when run by valgrind
        S.semantic_labels[0] = None
        I_free_signatures(ctypes.byref(S))
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
        S = Signature()
        I_init_signatures(ctypes.byref(S), 2)
        self.assertEqual(S.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 2)
        S.title = b"Signature title"
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.semantic_labels[1] = ctypes.create_string_buffer(b"The_Doors")
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
        ret = I_sort_signatures_by_semantic_label(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # semantic labels and sig items should be swapped
        # Static items
        self.assertEqual(S.sig[0].npoints, 69)
        self.assertEqual(S.sig[1].npoints, 42)
        # Reordered items
        semantic_label1 = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label1, "The_Doors")
        semantic_label2 = utils.decode(
            ctypes.cast(S.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label2, "The_Who")
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
        S.semantic_labels[0] = None
        S.semantic_labels[1] = None
        I_free_signatures(ctypes.byref(S))
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
        S = Signature()
        I_init_signatures(ctypes.byref(S), 2)
        self.assertEqual(S.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 2)
        S.title = b"Signature title"
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.semantic_labels[1] = ctypes.create_string_buffer(b"The_Doors")
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
        ret = I_sort_signatures_by_semantic_label(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # semantic labels and sig items should not be swapped
        # Static items
        self.assertEqual(S.sig[0].npoints, 69)
        self.assertEqual(S.sig[1].npoints, 42)
        # Reordered items
        semantic_label1 = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label1, "The_Who")
        semantic_label2 = utils.decode(
            ctypes.cast(S.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label2, "The_Doors")
        self.assertEqual(S.sig[0].mean[0], 3.5)
        self.assertEqual(S.sig[0].var[0][0], 1.7)
        self.assertEqual(S.sig[1].mean[0], 2.5)
        self.assertEqual(S.sig[1].var[0][0], 0.7)

        # Clean up memory to help track memory leaks when run by valgrind
        S.semantic_labels[0] = None
        S.semantic_labels[1] = None
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)

    def test_complete_match_reorder(self):
        # Prepare imagery group reference struct
        R = Ref()
        I_init_group_ref(ctypes.byref(R))
        ret = I_add_file_to_group_ref(self.map1, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 0)
        ret = I_add_file_to_group_ref(self.map2, self.mapset, ctypes.byref(R))
        self.assertEqual(ret, 1)

        # Prepare signature struct
        S = Signature()
        I_init_signatures(ctypes.byref(S), 2)
        self.assertEqual(S.nbands, 2)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 1)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 2)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 3)
        sig_count = I_new_signature(ctypes.byref(S))
        self.assertEqual(sig_count, 4)
        S.title = b"Signature title"
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.semantic_labels[1] = ctypes.create_string_buffer(b"The_Doors")
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
        S.sig[2].status = 1
        S.sig[2].have_color = 0
        S.sig[2].npoints = 12
        S.sig[2].desc = b"my label4"
        S.sig[2].mean[0] = 5.5
        S.sig[2].mean[1] = 9.9
        S.sig[2].var[0][0] = 0.9
        S.sig[2].var[1][0] = 0.8
        S.sig[2].var[1][1] = 0.7
        S.sig[3].status = 1
        S.sig[3].have_color = 0
        S.sig[3].npoints = 21
        S.sig[3].desc = b"my label3"
        S.sig[3].mean[0] = 9.9
        S.sig[3].mean[1] = 3.3
        S.sig[3].var[0][0] = 0.8
        S.sig[3].var[1][0] = 0.7
        S.sig[3].var[1][1] = 0.6

        # This should result in returning NULL
        ret = I_sort_signatures_by_semantic_label(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # semantic labels and sig items should be swapped
        # Static items
        self.assertEqual(S.sig[0].npoints, 69)
        self.assertEqual(S.sig[1].npoints, 42)
        self.assertEqual(S.sig[2].npoints, 12)
        self.assertEqual(S.sig[3].npoints, 21)
        # Reordered items
        semantic_label1 = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label1, "The_Doors")
        semantic_label2 = utils.decode(
            ctypes.cast(S.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label2, "The_Who")
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
        self.assertEqual(S.sig[2].mean[0], 9.9)
        self.assertEqual(S.sig[2].mean[1], 5.5)
        self.assertEqual(S.sig[2].var[0][0], 0.7)
        self.assertEqual(S.sig[2].var[1][0], 0.8)
        self.assertEqual(S.sig[2].var[1][1], 0.9)
        self.assertEqual(S.sig[3].mean[0], 3.3)
        self.assertEqual(S.sig[3].mean[1], 9.9)
        self.assertEqual(S.sig[3].var[0][0], 0.6)
        self.assertEqual(S.sig[3].var[1][0], 0.7)
        self.assertEqual(S.sig[3].var[1][1], 0.8)

        # Clean up memory to help track memory leaks when run by valgrind
        S.semantic_labels[0] = None
        S.semantic_labels[1] = None
        I_free_signatures(ctypes.byref(S))
        I_free_group_ref(ctypes.byref(R))
        if ret:
            if ret[0]:
                self.libc.free(ret[0])
            if ret[1]:
                self.libc.free(ret[1])
        self.libc.free(ret)


if __name__ == "__main__":
    test()
