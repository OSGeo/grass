"""
Name:      r.in.pdal filter and selection test
Purpose:   Validates output of input filtering and dimension selection

Author:    Maris Nartiss
Copyright: (C) 2020 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import pathlib
import shutil
import unittest
from tempfile import TemporaryDirectory

from grass.script import core as grass
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class SelectionTest(TestCase):
    """Test input dimension selection and filtering

    This test requires pdal CLI util to be available.
    This tests expects r.in.ascii to work properly.
    """

    @classmethod
    @unittest.skipIf(shutil.which("pdal") is None, "Cannot find pdal utility")
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", n=18, s=0, e=18, w=0, res=6)

        cls.data_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "data")
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

    def tearDown(self):
        """Remove the outputs created by the import

        This is executed after each test run.
        """
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(self.imp_raster, self.ref_raster),
        )
        try:
            self.runModule("g.remove", flags="f", type="raster", name=self.base_raster)
        except AttributeError:
            pass

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_dimension(self):
        """Test LAS dimension selection"""
        self.imp_raster = "imp_intensity"
        self.ref_raster = "ref_intensity"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.imp_raster,
            flags="o",
            quiet=True,
            method="mean",
            type="CELL",
            dimension="intensity",
        )
        self.assertRasterExists(self.imp_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_mean_intensity.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.imp_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_user_dimension(self):
        """Test PDAL user dimension selection"""
        self.imp_raster = "imp_cellid"
        self.ref_raster = "ref_cellid"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.imp_raster,
            flags="o",
            quiet=True,
            method="mode",
            type="CELL",
            user_dimension="CellID",
        )
        self.assertRasterExists(self.imp_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_mode_cellid.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.imp_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_filter(self):
        """Test input filtering"""
        self.imp_raster = "imp_filtered"
        self.ref_raster = "ref_filtered"

        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.imp_raster,
            flags="o",
            quiet=True,
            method="mode",
            type="CELL",
            dimension="source",
            zrange=(2, 10),
            irange=(10, 20),
            drange=(1, 1),
        )
        self.assertRasterExists(self.imp_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_filter_z_int_source.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.imp_raster, self.ref_raster, 0)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_base_raster(self):
        """Test Z adjustment by base raster"""
        self.imp_raster = "imp_base_adj"
        self.ref_raster = "ref_base_adj"
        self.base_raster = "base_raster"

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_mean_z.ascii"),
            output=self.base_raster,
        )
        self.assertModule(
            "r.in.pdal",
            input=self.las_file,
            output=self.imp_raster,
            flags="o",
            quiet=True,
            method="max",
            base_raster=self.base_raster,
        )
        self.assertRasterExists(self.imp_raster)

        self.runModule(
            "r.in.ascii",
            input=os.path.join(self.data_dir, "res_base_adj.ascii"),
            output=self.ref_raster,
        )
        self.assertRastersEqual(self.imp_raster, self.ref_raster, 0.001)


if __name__ == "__main__":
    test()
