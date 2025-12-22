"""
Name:      r.in.pdal binning method test
Purpose:   Validates output of various binning methods

Author:    Maris Nartiss
Copyright: (C) 2021 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import unittest
import shutil
from pathlib import Path
from tempfile import TemporaryDirectory

from grass.script import core as grass
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class BinningTest(TestCase):
    """Test binning methods

    This test requires pdal CLI util to be available.
    This tests expects r.in.ascii to work properly.
    """

    @classmethod
    @unittest.skipIf(shutil.which("pdal") is None, "Cannot find pdal utility")
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", n=18, s=0, e=18, w=0, res=6)

        cls.data_dir = os.path.join(Path(__file__).parent.absolute(), "data")
        cls.point_file = os.path.join(cls.data_dir, "points.csv")
        cls.tmp_dir = TemporaryDirectory()
        cls.las_file = os.path.join(cls.tmp_dir.name, "points.las")
        grass.call(
            [
                "pdal",
                "translate",
                "-i",
                cls.point_file,
                "-o",
                cls.las_file,
                "-r",
                "text",
                "-w",
                "las",
                "--writers.las.format=0",
                "--writers.las.extra_dims=all",
                "--writers.las.minor_version=4",
            ]
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.tmp_dir.cleanup()
        cls.del_temp_region()

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def tearDown(self):
        """Remove the outputs created by the import

        This is executed after each test run.
        """
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(self.bin_raster, self.ref_raster),
        )

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_n(self):
        """Test binning with n method"""
        self.bin_raster = "bin_n"
        self.ref_raster = "ref_n"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="n",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_n_all.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_min(self):
        self.bin_raster = "bin_min"
        self.ref_raster = "ref_min"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="min",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_min_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_max(self):
        self.bin_raster = "bin_max"
        self.ref_raster = "ref_max"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="max",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_max_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_range(self):
        self.bin_raster = "bin_range"
        self.ref_raster = "ref_range"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="range",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_range_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_sum(self):
        self.bin_raster = "bin_sum"
        self.ref_raster = "ref_sum"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="sum",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_sum_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_mean(self):
        self.bin_raster = "bin_mean"
        self.ref_raster = "ref_mean"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="mean",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_mean_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0.0001)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_stddev(self):
        self.bin_raster = "bin_stddev"
        self.ref_raster = "ref_stddev"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="stddev",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_stddev_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0.0001)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_variance(self):
        self.bin_raster = "bin_variance"
        self.ref_raster = "ref_variance"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="variance",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_variance_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0.0001)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_coeff_var(self):
        self.bin_raster = "bin_coeff_var"
        self.ref_raster = "ref_coeff_var"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="coeff_var",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_coeff_var_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0.0001)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_median(self):
        self.bin_raster = "bin_median"
        self.ref_raster = "ref_median"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="median",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_median_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0.0001)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_mode(self):
        self.bin_raster = "bin_mode"
        self.ref_raster = "ref_mode"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="mode",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_mode_z.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_sidnmax(self):
        self.bin_raster = "bin_sidnmax"
        self.ref_raster = "ref_sidnmax"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="sidnmax",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_sidnmax_all.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_method_sidnmin(self):
        self.bin_raster = "bin_sidnmin"
        self.ref_raster = "ref_sidnmin"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.bin_raster,
            flags="o",
            quiet=True,
            method="sidnmin",
        )
        self.assertRasterExists(self.bin_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_sidnmin_all.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.bin_raster, self.ref_raster, 0)


if __name__ == "__main__":
    test()
