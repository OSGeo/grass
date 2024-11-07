"""
Name:      i.gensig general functionality
Purpose:   Test ability to generate signature files

Author:    Maris Nartiss
Copyright: (C) 2022 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import stat
import ctypes
import shutil
from pathlib import Path

from grass.pygrass import utils
from grass.pygrass.gis import Mapset
from grass.script.core import tempname

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.lib.gis import G_mapset_path
from grass.lib.raster import Rast_write_semantic_label
from grass.lib.imagery import (
    Signature,
    Ref,
    I_init_group_ref,
    I_add_file_to_group_ref,
    I_put_group_ref,
    I_put_subgroup_ref,
    I_fopen_signature_file_old,
    I_read_signatures,
)


class SuccessTest(TestCase):
    """Test successful generation of a sig file"""

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", n=5, s=0, e=5, w=0, res=1)
        cls.data_dir = "data"
        cls.mpath = utils.decode(G_mapset_path())
        cls.mapset_name = Mapset().name

        # Load imagery bands and training set
        cls.b1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "b1.ascii"),
            output=cls.b1,
        )
        cls.b2 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "b2.ascii"),
            output=cls.b2,
        )
        cls.train = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "train.ascii"),
            output=cls.train,
        )
        cls.group = tempname(10)
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

        # Location of target signature file
        cls.sig_name1 = tempname(10)
        cls.sig_dir1 = f"{cls.mpath}/signatures/sig/{cls.sig_name1}"

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.del_temp_region()
        shutil.rmtree(cls.sig_dir1, ignore_errors=True)
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(cls.b1, cls.b2, cls.train),
            quiet=True,
        )

    def test_creation(self):
        """Test creating a signature"""
        self.assertModule(
            "i.gensig",
            trainingmap=self.train,
            group=self.group,
            subgroup=self.group,
            signaturefile=self.sig_name1,
            quiet=True,
        )

        # File must be present
        sig_stat = Path(self.sig_dir1, "sig").stat()
        self.assertTrue(stat.S_ISREG(sig_stat.st_mode))

        # Compare values within sig file
        Sn = Signature()
        p_old_sigfile = I_fopen_signature_file_old(self.sig_name1)
        ret = I_read_signatures(p_old_sigfile, ctypes.byref(Sn))
        self.assertEqual(ret, 1)
        self.assertEqual(Sn.nbands, 2)
        self.assertEqual(Sn.nsigs, 3)
        self.assertEqual(Sn.have_oclass, 1)
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[0], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, self.semantic_label1)
        semantic_label = utils.decode(
            ctypes.cast(Sn.semantic_labels[1], ctypes.c_char_p).value
        )
        self.assertEqual(semantic_label, self.semantic_label2)
        # 1
        self.assertEqual(Sn.sig[0].status, 1)
        self.assertEqual(Sn.sig[0].have_color, 0)
        self.assertEqual(Sn.sig[0].npoints, 4)
        self.assertEqual(Sn.sig[0].oclass, 1)
        self.assertTrue(abs(Sn.sig[0].mean[0] - 2.9) < 0.1)
        self.assertTrue(abs(Sn.sig[0].mean[1] - 7.0) < 0.1)
        self.assertTrue(abs(Sn.sig[0].var[0][0] - 0.03) < 0.01)
        self.assertTrue(abs(Sn.sig[0].var[1][1] - 0.6) < 0.1)
        # 6
        self.assertEqual(Sn.sig[1].status, 1)
        self.assertEqual(Sn.sig[1].have_color, 0)
        self.assertEqual(Sn.sig[1].npoints, 4)
        self.assertEqual(Sn.sig[1].oclass, 6)
        self.assertTrue(abs(Sn.sig[1].mean[0] - 8.9) < 0.1)
        self.assertTrue(abs(Sn.sig[1].mean[1] - 1.5) < 0.1)
        self.assertTrue(abs(Sn.sig[1].var[0][0] - 0.003) < 0.001)
        self.assertTrue(abs(Sn.sig[1].var[1][1] - 0.3) < 0.1)
        # 9
        self.assertEqual(Sn.sig[2].status, 1)
        self.assertEqual(Sn.sig[2].have_color, 0)
        self.assertEqual(Sn.sig[2].npoints, 3)
        self.assertEqual(Sn.sig[2].oclass, 9)
        self.assertTrue(abs(Sn.sig[2].mean[0] - 6.7) < 0.1)
        self.assertTrue(abs(Sn.sig[2].mean[1] - 8.3) < 0.1)
        self.assertTrue(abs(Sn.sig[2].var[0][0] - 0.043) < 0.01)
        self.assertTrue(abs(Sn.sig[2].var[1][1] - 0.3) < 0.1)


if __name__ == "__main__":
    test()
