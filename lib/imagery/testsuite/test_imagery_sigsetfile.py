"""Test of imagery library sigset file handling

@author Maris Nartiss

@copyright 2021 by Maris Nartiss and the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""
<<<<<<< HEAD
<<<<<<< HEAD

=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
import os
import stat
import ctypes
import shutil
<<<<<<< HEAD
<<<<<<< HEAD
=======
import os
import stat
import ctypes
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import tempname
from grass.pygrass import utils
from grass.pygrass.gis import Mapset

from grass.lib.gis import G_mapset_path
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
from grass.lib.raster import Rast_write_semantic_label
=======
from grass.lib.raster import Rast_write_bandref
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
from grass.lib.raster import Rast_write_semantic_label
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
from grass.lib.raster import Rast_write_semantic_label
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
from grass.lib.imagery import (
    SigSet,
    I_InitSigSet,
    I_NewClassSig,
    I_NewSubSig,
    I_WriteSigSet,
    I_ReadSigSet,
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    I_SortSigSetBySemanticLabel,
=======
    I_SortSigSetByBandref,
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    I_SortSigSetBySemanticLabel,
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    I_SortSigSetBySemanticLabel,
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    I_fopen_sigset_file_new,
    I_fopen_sigset_file_old,
    Ref,
    I_init_group_ref,
    I_add_file_to_group_ref,
    I_free_group_ref,
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    ReturnString,
=======
    String,
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    ReturnString,
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    ReturnString,
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
)


class SigSetFileTestCase(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sig_name = tempname(10)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        cls.sig_dir = f"{cls.mpath}/signatures/sigset/{cls.sig_name}"

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.sig_dir, ignore_errors=True)
<<<<<<< HEAD
=======
        cls.sigfile_name = f"{cls.mpath}/signatures/sigset/{cls.sig_name}"

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(cls.sigfile_name)
        except OSError:
            pass
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        cls.sig_dir = f"{cls.mpath}/signatures/sigset/{cls.sig_name}"

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.sig_dir, ignore_errors=True)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

    def test_I_fopen_signature_file_old_fail(self):
        sigfile = I_fopen_sigset_file_old(tempname(10))
        self.assertFalse(sigfile)

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    def test_roundtrip_sigset_v1_one_label(self):
        """Test writing and reading back sigset file (v1)
        with a single label and fully qualified sigfile name"""
=======
    def test_roundtrip_sigset_v1_one_band(self):
        """Test writing and reading back sigset file (v1)
        with a single band and fully qualified sigfile name"""
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    def test_roundtrip_sigset_v1_one_label(self):
        """Test writing and reading back sigset file (v1)
        with a single label and fully qualified sigfile name"""
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    def test_roundtrip_sigset_v1_one_label(self):
        """Test writing and reading back sigset file (v1)
        with a single label and fully qualified sigfile name"""
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        # Create signature struct
        So = SigSet()
        I_InitSigSet(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
        self.assertEqual(So.ClassSig[0].nsubclasses, 1)

        # Fill sigset struct with data
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        So.title = ReturnString("Signature title")
        So.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        So.title = String("Signature title")
        So.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        So.title = ReturnString("Signature title")
        So.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        So.ClassSig[0].type = 1
        So.ClassSig[0].SubSig[0].pi = 3.14
        So.ClassSig[0].SubSig[0].means[0] = 42.42
        So.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # Write signatures to file
        p_new_sigfile = I_fopen_sigset_file_new(self.sig_name)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        sig_stat = os.stat(f"{self.sig_dir}/sig")
=======
        sig_stat = os.stat(self.sigfile_name)
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        sig_stat = os.stat(f"{self.sig_dir}/sig")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        sig_stat = os.stat(f"{self.sig_dir}/sig")
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))
        I_WriteSigSet(p_new_sigfile, ctypes.byref(So))
        self.libc.fclose(p_new_sigfile)

        # Read back from signatures file
        Sn = SigSet()
        fq_name = f"{self.sig_name}@{self.mapset_name}"
        p_old_sigfile = I_fopen_sigset_file_old(fq_name)
        ret = I_ReadSigSet(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, 1)
        self.assertEqual(utils.decode(Sn.title), "Signature title")
        self.assertEqual(Sn.nbands, 1)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
<<<<<<< HEAD
<<<<<<< HEAD
=======
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    def test_read_fail_sigset_v1_one_label(self):
        """Reading back should fail as semantic label length exceeds limit"""
=======
    def test_read_fail_sigset_v1_one_band(self):
        """Reading back should fail as band reference exceeds limit"""
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    def test_read_fail_sigset_v1_one_label(self):
        """Reading back should fail as semantic label length exceeds limit"""
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    def test_read_fail_sigset_v1_one_label(self):
        """Reading back should fail as semantic label length exceeds limit"""
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        # Create signature struct
        So = SigSet()
        I_InitSigSet(ctypes.byref(So), 1)
        self.assertEqual(So.nbands, 1)
        I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
        self.assertEqual(So.ClassSig[0].nsubclasses, 1)

        # Fill sigset struct with data
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        So.title = ReturnString("Signature title")
        So.semantic_labels[0] = ctypes.create_string_buffer(tempname(252).encode())
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        So.title = String("Signature title")
        So.bandrefs[0] = ctypes.create_string_buffer(tempname(252).encode())
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        So.title = ReturnString("Signature title")
        So.semantic_labels[0] = ctypes.create_string_buffer(tempname(252).encode())
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        So.ClassSig[0].type = 1
        So.ClassSig[0].SubSig[0].pi = 3.14
        So.ClassSig[0].SubSig[0].means[0] = 42.42
        So.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # Write signatures to file
        p_new_sigfile = I_fopen_sigset_file_new(self.sig_name)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        sig_stat = os.stat(f"{self.sig_dir}/sig")
=======
        sig_stat = os.stat(self.sigfile_name)
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        sig_stat = os.stat(f"{self.sig_dir}/sig")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        sig_stat = os.stat(f"{self.sig_dir}/sig")
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    def test_roundtrip_sigset_v1_two_labels(self):
        """Test writing and reading back sigset (v1) with two labels"""
=======
    def test_roundtrip_sigset_v1_two_bands(self):
        """Test writing and reading back sigset (v1) with two bands"""
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    def test_roundtrip_sigset_v1_two_labels(self):
        """Test writing and reading back sigset (v1) with two labels"""
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    def test_roundtrip_sigset_v1_two_labels(self):
        """Test writing and reading back sigset (v1) with two labels"""
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

        # Create signature struct
        So = SigSet()
        I_InitSigSet(ctypes.byref(So), 2)
        self.assertEqual(So.nbands, 2)
        I_NewClassSig(ctypes.byref(So))
        self.assertEqual(So.nclasses, 1)
        I_NewSubSig(ctypes.byref(So), ctypes.byref(So.ClassSig[0]))
        self.assertEqual(So.ClassSig[0].nsubclasses, 1)

        # Fill sigset struct with data
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        So.title = ReturnString("Signature title")
        So.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        So.semantic_labels[1] = ctypes.create_string_buffer(b"The_Who")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        So.title = String("Signature title")
        So.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        So.bandrefs[1] = ctypes.create_string_buffer(b"The_Who")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        So.title = ReturnString("Signature title")
        So.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        So.semantic_labels[1] = ctypes.create_string_buffer(b"The_Who")
        So.ClassSig[0].used = 1
        So.ClassSig[0].classnum = 2
        So.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        sig_stat = os.stat(f"{self.sig_dir}/sig")
=======
        sig_stat = os.stat(self.sigfile_name)
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        sig_stat = os.stat(f"{self.sig_dir}/sig")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        sig_stat = os.stat(f"{self.sig_dir}/sig")
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Who")
<<<<<<< HEAD
<<<<<<< HEAD
=======
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
        bandref = utils.decode(ctypes.cast(Sn.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Who")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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


<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
class SortSigSetBySemanticLabelTest(TestCase):
=======
class SortSigSetByBandrefTest(TestCase):
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
class SortSigSetBySemanticLabelTest(TestCase):
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
class SortSigSetBySemanticLabelTest(TestCase):
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    @classmethod
    def setUpClass(cls):
        cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mapset = Mapset().name
        cls.map1 = tempname(10)
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        cls.semantic_label1 = "The_Doors"
        cls.map2 = tempname(10)
        cls.semantic_label2 = "The_Who"
=======
        cls.bandref1 = "The_Doors"
        cls.map2 = tempname(10)
        cls.bandref2 = "The_Who"
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        cls.semantic_label1 = "The_Doors"
        cls.map2 = tempname(10)
        cls.semantic_label2 = "The_Who"
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        cls.semantic_label1 = "The_Doors"
        cls.map2 = tempname(10)
        cls.semantic_label2 = "The_Who"
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        cls.map3 = tempname(10)
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression=f"{cls.map1} = 1")
        cls.runModule("r.mapcalc", expression=f"{cls.map2} = 1")
        cls.runModule("r.mapcalc", expression=f"{cls.map3} = 1")
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        Rast_write_semantic_label(cls.map1, cls.semantic_label1)
        Rast_write_semantic_label(cls.map2, cls.semantic_label2)
=======
        Rast_write_bandref(cls.map1, cls.bandref1)
        Rast_write_bandref(cls.map2, cls.bandref2)
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        Rast_write_semantic_label(cls.map1, cls.semantic_label1)
        Rast_write_semantic_label(cls.map2, cls.semantic_label2)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        Rast_write_semantic_label(cls.map1, cls.semantic_label1)
        Rast_write_semantic_label(cls.map2, cls.semantic_label2)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in two error strings in ret
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
=======
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Troggs")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in two error strings in ret
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
=======
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    def test_missing_label(self):
=======
    def test_missing_bandref(self):
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
    def test_missing_label(self):
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
    def test_missing_label(self):
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in two error strings in ret
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
=======
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        self.assertTrue(bool(ret))
        sig_err = utils.decode(ctypes.cast(ret[0], ctypes.c_char_p).value)
        ref_err = utils.decode(ctypes.cast(ret[1], ctypes.c_char_p).value)
        self.assertEqual(
            sig_err,
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
            "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>,<semantic label missing>,"
            + "<semantic label missing>",
<<<<<<< HEAD
<<<<<<< HEAD
        )
        self.assertEqual(ref_err, f"The_Doors,{self.map3}")
=======
            "<band reference missing>,<band reference missing>,"
            + "<band reference missing>,<band reference missing>,"
            + "<band reference missing>,<band reference missing>,"
            + "<band reference missing>,<band reference missing>,"
            + "<band reference missing>",
        )
        self.assertEqual(ref_err, "The_Doors,<band reference missing>")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        )
        self.assertEqual(ref_err, f"The_Doors,{self.map3}")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
        )
        self.assertEqual(ref_err, f"The_Doors,{self.map3}")
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69

        # This should result in returning NULL
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        semantic_label = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
<<<<<<< HEAD
=======
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        bandref = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref, "The_Doors")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        semantic_label = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, "The_Doors")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.semantic_labels[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.bandrefs[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.semantic_labels[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].means[1] = 24.24
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69
        S.ClassSig[0].SubSig[0].R[0][1] = 96.96
        S.ClassSig[0].SubSig[0].R[1][0] = -69.69
        S.ClassSig[0].SubSig[0].R[1][1] = -96.96

        # This should result in returning NULL
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # Semantic labels and sig items should be swapped
        # Static items
        self.assertEqual(S.ClassSig[0].SubSig[0].pi, 3.14)
        # Reordered items
        semantic_label1 = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label1, "The_Doors")
        semantic_label2 = utils.decode(
            ctypes.cast(S.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label2, "The_Who")
<<<<<<< HEAD
=======
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
        self.assertFalse(bool(ret))
        # Semantic labels and sig items should be swapped
        # Static items
        self.assertEqual(S.ClassSig[0].SubSig[0].pi, 3.14)
        # Reordered items
<<<<<<< HEAD
        bandref1 = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref1, "The_Doors")
        bandref2 = utils.decode(ctypes.cast(S.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref2, "The_Who")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        semantic_label1 = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label1, "The_Doors")
        semantic_label2 = utils.decode(
            ctypes.cast(S.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label2, "The_Who")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.semantic_labels[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
<<<<<<< HEAD
=======
        S.title = String("Signature title")
        S.bandrefs[0] = ctypes.create_string_buffer(b"The_Who")
        S.bandrefs[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = String("1st class")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        S.title = ReturnString("Signature title")
        S.semantic_labels[0] = ctypes.create_string_buffer(b"The_Who")
        S.semantic_labels[1] = ctypes.create_string_buffer(b"The_Doors")
        S.ClassSig[0].used = 1
        S.ClassSig[0].classnum = 2
        S.ClassSig[0].title = ReturnString("1st class")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        S.ClassSig[0].type = 1
        S.ClassSig[0].SubSig[0].pi = 3.14
        S.ClassSig[0].SubSig[0].means[0] = 42.42
        S.ClassSig[0].SubSig[0].means[1] = 24.24
        S.ClassSig[0].SubSig[0].R[0][0] = 69.69
        S.ClassSig[0].SubSig[0].R[0][1] = 96.96
        S.ClassSig[0].SubSig[0].R[1][0] = -69.69
        S.ClassSig[0].SubSig[0].R[1][1] = -96.96

        # This should result in returning NULL
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
        self.assertFalse(bool(ret))
        # Semantic labels and sig items should not be swapped
        # Static items
        self.assertEqual(S.ClassSig[0].SubSig[0].pi, 3.14)
        # Reordered items
        semantic_label1 = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label1, "The_Who")
        semantic_label2 = utils.decode(
            ctypes.cast(S.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label2, "The_Doors")
<<<<<<< HEAD
=======
        ret = I_SortSigSetByBandref(ctypes.byref(S), ctypes.byref(R))
=======
        ret = I_SortSigSetBySemanticLabel(ctypes.byref(S), ctypes.byref(R))
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
        self.assertFalse(bool(ret))
        # Semantic labels and sig items should not be swapped
        # Static items
        self.assertEqual(S.ClassSig[0].SubSig[0].pi, 3.14)
        # Reordered items
<<<<<<< HEAD
        bandref1 = utils.decode(ctypes.cast(S.bandrefs[0], ctypes.c_char_p).value)
        self.assertEqual(bandref1, "The_Who")
        bandref2 = utils.decode(ctypes.cast(S.bandrefs[1], ctypes.c_char_p).value)
        self.assertEqual(bandref2, "The_Doors")
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
        semantic_label1 = utils.decode(
            ctypes.cast(S.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label1, "The_Who")
        semantic_label2 = utils.decode(
            ctypes.cast(S.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label2, "The_Doors")
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
