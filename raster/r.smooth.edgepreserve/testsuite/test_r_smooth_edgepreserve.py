"""
Name:      r.smooth.edgepreserve tests
Purpose:   Test corectness of outputs

Author:    Maris Nartiss
Copyright: (C) 2025 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
from pathlib import Path

from grass.script.core import tempname
from grass.script.raster import raster_info
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class SmoothingTest(TestCase):
    """
    Test correctness of smoothing implementations against hand crafted
    reference data
    """

    @classmethod
    def setUpClass(cls):
        """Import sample maps with known properties"""

        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)

        cls.data_dir = os.path.join(Path(__file__).parent.absolute(), "data")
        cls.rm_rast = []

        cls.in_map_i = tempname(10)
        cls.rm_rast.append(cls.in_map_i)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "input1.ascii"),
            output=cls.in_map_i,
        )
        cls.in_map_f = tempname(10)
        cls.rm_rast.append(cls.in_map_f)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.in_map_f}=float({cls.in_map_i})",
        )
        cls.in_map_d = tempname(10)
        cls.rm_rast.append(cls.in_map_d)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.in_map_d}=double({cls.in_map_i})",
        )
        cls.ref_exp_f = tempname(10)
        cls.rm_rast.append(cls.ref_exp_f)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_exp.ascii"),
            output=cls.ref_exp_f,
        )
        cls.ref_exp_i = tempname(10)
        cls.rm_rast.append(cls.ref_exp_i)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.ref_exp_i}=round({cls.ref_exp_f})",
        )
        cls.ref_quad_f = tempname(10)
        cls.rm_rast.append(cls.ref_quad_f)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_quad.ascii"),
            output=cls.ref_quad_f,
        )
        cls.ref_tuk_f = tempname(10)
        cls.rm_rast.append(cls.ref_tuk_f)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_tuk.ascii"),
            output=cls.ref_tuk_f,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data"""

        cls.del_temp_region()
        for r in cls.rm_rast:
            cls.runModule("g.remove", flags="f", type="raster", name=r)

    def test_exp_i_ram(self):
        """
        Test smoothing with Perona & Malik proposed exponential
        diffusivity function while keeping data in RAM.
        Also tests conversion to int.
        """

        out_map_i = tempname(10)
        self.rm_rast.append(out_map_i)
        self.assertModule(
            "r.smooth.edgepreserve",
            input_=self.in_map_i,
            output=out_map_i,
            memory=100,
            steps=5,
            threshold=2,
            lambda_=0.25,
            function="exp",
            quiet=True,
        )
        self.assertTrue(raster_info(out_map_i)["datatype"] == "CELL")
        self.assertRastersEqual(out_map_i, self.ref_exp_i)

    def test_exp_f_ram(self):
        """
        Test smoothing with Perona & Malik proposed exponential
        diffusivity function while keeping data in RAM.
        Also tests conversion to float.
        """

        out_map_f = tempname(10)
        self.rm_rast.append(out_map_f)
        self.assertModule(
            "r.smooth.edgepreserve",
            input_=self.in_map_f,
            output=out_map_f,
            memory=100,
            steps=5,
            threshold=2,
            lambda_=0.25,
            function="exp",
            quiet=True,
        )
        self.assertTrue(raster_info(out_map_f)["datatype"] == "FCELL")
        self.assertRastersEqual(out_map_f, self.ref_exp_f, precision=0.1)

    def test_exp_d_ram(self):
        """
        Test smoothing with Perona & Malik proposed exponential
        diffusivity function while keeping data in RAM.
        Also tests conversion to double.
        """

        out_map_d = tempname(10)
        self.rm_rast.append(out_map_d)
        self.assertModule(
            "r.smooth.edgepreserve",
            input_=self.in_map_d,
            output=out_map_d,
            memory=100,
            steps=5,
            threshold=2,
            lambda_=0.25,
            function="exp",
            quiet=True,
        )
        self.assertTrue(raster_info(out_map_d)["datatype"] == "DCELL")
        self.assertRastersEqual(out_map_d, self.ref_exp_f, precision=0.1)

    def test_quad_ram(self):
        """
        Test smoothing with Perona & Malik proposed quadratic
        diffusivity function while keeping data in RAM
        """

        out_map_f = tempname(10)
        self.rm_rast.append(out_map_f)
        self.assertModule(
            "r.smooth.edgepreserve",
            input_=self.in_map_f,
            output=out_map_f,
            memory=100,
            steps=5,
            threshold=2,
            lambda_=0.25,
            function="quad",
            quiet=True,
        )
        self.assertRastersEqual(out_map_f, self.ref_quad_f, precision=0.1)

    def test_tuk_ram(self):
        """
        Test smoothing with Black et al. proposed Tukey  diffusivity
        function while keeping data in RAM
        """

        out_map_f = tempname(10)
        self.rm_rast.append(out_map_f)
        self.assertModule(
            "r.smooth.edgepreserve",
            input_=self.in_map_f,
            output=out_map_f,
            memory=100,
            steps=5,
            threshold=2,
            lambda_=0.25,
            function="tuk",
            quiet=True,
        )
        self.assertRastersEqual(out_map_f, self.ref_tuk_f, precision=0.1)

    def test_exp_disk(self):
        """
        Test smoothing with Perona & Malik proposed exponential
        diffusivity function while storing data on disk.
        """

        out_map_f = tempname(10)
        self.rm_rast.append(out_map_f)
        self.assertModule(
            "r.smooth.edgepreserve",
            input_=self.in_map_f,
            output=out_map_f,
            memory=0.0022,
            steps=5,
            threshold=2,
            lambda_=0.25,
            function="exp",
            quiet=True,
        )
        self.assertRastersEqual(out_map_f, self.ref_exp_f, precision=0.1)


if __name__ == "__main__":
    test()
