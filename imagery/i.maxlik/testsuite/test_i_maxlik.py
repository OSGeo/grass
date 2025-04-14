"""
Name:      i.maxlik general functionality
Purpose:   Test ability to perform classification

Author:    Maris Nartiss
Copyright: (C) 2022 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import ctypes
import os
import shutil

from grass.pygrass import utils
from grass.pygrass.gis import Mapset
from grass.pygrass import raster
from grass.script.core import tempname

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows

from grass.lib.gis import G_mapset_path
from grass.lib.raster import Rast_write_semantic_label
from grass.lib.imagery import (
    Signature,
    Ref,
    I_init_signatures,
    I_new_signature,
    I_fopen_signature_file_new,
    I_write_signatures,
    I_init_group_ref,
    I_add_file_to_group_ref,
    I_put_group_ref,
    I_put_subgroup_ref,
)


class SuccessTest(TestCase):
    """Test successful classification"""

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", n=5, s=0, e=5, w=0, res=1)
        if os.name == "nt":
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("msvcrt"))
        else:
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name
        cls.sig_name1 = tempname(10)
        cls.sig_dir1 = f"{cls.mpath}/signatures/sig/{cls.sig_name1}"
        cls.sig_name2 = tempname(10)
        cls.sig_dir2 = f"{cls.mpath}/signatures/sig/{cls.sig_name2}"
        cls.v1_class = tempname(10)
        cls.v2_class = tempname(10)

        # Imagery group to classify
        cls.b1 = tempname(10)
        cls.b2 = tempname(10)
        cls.group = tempname(10)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.b1}=10.0+rand(-1.0,1.0)",
            flags="s",
            quiet=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.b2}=if(row() == 3 && col() == 3, null(), 5.0+rand(-1.0,1.0))",
            flags="s",
            quiet=True,
        )
        cls.semantic_label1 = "The_Doors"
        cls.semantic_label2 = "The_Who"
        Rast_write_semantic_label(cls.b1, cls.semantic_label1)
        Rast_write_semantic_label(cls.b2, cls.semantic_label2)
        Rg = Ref()
        I_init_group_ref(ctypes.byref(Rg))
        I_add_file_to_group_ref(cls.b1, cls.mapset_name, ctypes.byref(Rg))
        I_add_file_to_group_ref(cls.b2, cls.mapset_name, ctypes.byref(Rg))
        I_put_group_ref(cls.group, ctypes.byref(Rg))
        Rs = Ref()
        I_init_group_ref(ctypes.byref(Rs))
        I_add_file_to_group_ref(cls.b1, cls.mapset_name, ctypes.byref(Rs))
        I_add_file_to_group_ref(cls.b2, cls.mapset_name, ctypes.byref(Rs))
        I_put_subgroup_ref(cls.group, cls.group, ctypes.byref(Rs))

        # Old (v1) signature file
        So = Signature()
        I_init_signatures(ctypes.byref(So), 2)
        I_new_signature(ctypes.byref(So))
        I_new_signature(ctypes.byref(So))
        So.title = b"V1 signature"
        So.semantic_labels[0] = ctypes.create_string_buffer(
            cls.semantic_label1.encode()
        )
        So.semantic_labels[1] = ctypes.create_string_buffer(
            cls.semantic_label2.encode()
        )
        So.sig[0].status = 1
        So.sig[0].have_color = 0
        So.sig[0].npoints = 42
        So.sig[0].desc = b"my label"
        So.sig[0].mean[0] = 10
        So.sig[0].mean[1] = 5
        So.sig[0].var[0][0] = 1.5
        So.sig[0].var[1][0] = 2.1
        So.sig[0].var[1][1] = 5.7
        So.sig[1].status = 1
        So.sig[1].have_color = 0
        So.sig[1].npoints = 69
        So.sig[1].desc = b"not present"
        So.sig[1].mean[0] = 50
        So.sig[1].mean[1] = 75
        So.sig[1].var[0][0] = 5.5
        So.sig[1].var[1][0] = 8.1
        So.sig[1].var[1][1] = 12.7
        p_new_sigfile = I_fopen_signature_file_new(cls.sig_name1)
        I_write_signatures(p_new_sigfile, ctypes.byref(So))
        cls.libc.fclose(p_new_sigfile)

        # New (v2) signature file
        So = Signature()
        I_init_signatures(ctypes.byref(So), 2)
        I_new_signature(ctypes.byref(So))
        I_new_signature(ctypes.byref(So))
        So.title = b"V2 signature"
        So.semantic_labels[0] = ctypes.create_string_buffer(
            cls.semantic_label1.encode()
        )
        So.semantic_labels[1] = ctypes.create_string_buffer(
            cls.semantic_label2.encode()
        )
        So.have_oclass = 1
        So.sig[0].status = 1
        So.sig[0].have_color = 0
        So.sig[0].npoints = 42
        So.sig[0].oclass = 420
        So.sig[0].desc = b"my label"
        So.sig[0].mean[0] = 10
        So.sig[0].mean[1] = 5
        So.sig[0].var[0][0] = 1.5
        So.sig[0].var[1][0] = 2.1
        So.sig[0].var[1][1] = 5.7
        So.sig[1].status = 1
        So.sig[1].have_color = 0
        So.sig[1].npoints = 69
        So.sig[1].oclass = 690
        So.sig[1].desc = b"not present"
        So.sig[1].mean[0] = 50
        So.sig[1].mean[1] = 75
        So.sig[1].var[0][0] = 5.5
        So.sig[1].var[1][0] = 8.1
        So.sig[1].var[1][1] = 12.7
        p_new_sigfile = I_fopen_signature_file_new(cls.sig_name2)
        I_write_signatures(p_new_sigfile, ctypes.byref(So))
        cls.libc.fclose(p_new_sigfile)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.del_temp_region()
        shutil.rmtree(cls.sig_dir1, ignore_errors=True)
        shutil.rmtree(cls.sig_dir2, ignore_errors=True)
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(cls.b1, cls.b2, cls.v1_class, cls.v2_class),
            quiet=True,
        )
        cls.runModule("g.remove", flags="f", type="group", name=cls.group, quiet=True)

    @xfail_windows
    def test_v1(self):
        """Test v1 signature"""
        self.assertModule(
            "i.maxlik",
            group=self.group,
            subgroup=self.group,
            signaturefile=self.sig_name1,
            output=self.v1_class,
            quiet=True,
        )
        self.assertRasterExists(self.v1_class)
        self.assertRasterMinMax(
            map=self.v1_class, refmin=1, refmax=1, msg="Wrong predicted value"
        )
        res = raster.RasterRow(self.v1_class)
        res.open()
        self.assertTrue(res.has_cats())
        self.assertEqual(res.get_cat(0)[0], "my label")
        self.assertEqual(res.get_cat(0)[1], 1)
        res.close()

    @xfail_windows
    def test_v2(self):
        """Test v2 signature"""
        self.assertModule(
            "i.maxlik",
            group=self.group,
            subgroup=self.group,
            signaturefile=self.sig_name2,
            output=self.v2_class,
            quiet=True,
        )
        self.assertRasterExists(self.v2_class)
        self.assertRasterMinMax(
            map=self.v2_class, refmin=420, refmax=420, msg="Wrong predicted value"
        )
        res = raster.RasterRow(self.v2_class)
        res.open()
        self.assertTrue(res.has_cats())
        self.assertEqual(res.get_cat(0)[0], "my label")
        self.assertEqual(res.get_cat(0)[1], 420)
        res.close()


if __name__ == "__main__":
    test()
