"""
Name:      r.in.pdal info printing and error handling tests
Purpose:   Validates output of LAS file property printing and handling
           of broken LAS files

Author:    Maris Nartiss
Copyright: (C) 2024 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import shutil
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from grass.script import core as grass
from grass.script import read_command
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class InfoTest(TestCase):
    """
    Test printing of extent and metadata

    This test requires pdal CLI util to be available.
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
        cls.broken_las = os.path.join(cls.tmp_dir.name, "broken.las")
        Path(cls.broken_las).write_bytes(b"LASF")

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.tmp_dir.cleanup()
        cls.del_temp_region()

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_extent_bad(self):
        """A broken LAS file should result in an error"""
        self.assertModuleFail("r.in.pdal", input=self.broken_las, flags="g", quiet=True)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_info_bad(self):
        """A broken LAS file should result in an error"""
        self.assertModuleFail("r.in.pdal", input=self.broken_las, flags="p", quiet=True)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_extent_good(self):
        """Reported extent should match provided data"""
        out = read_command("r.in.pdal", input=self.las_file, flags="g", quiet=True)
        for kvp in out.strip().split(" "):
            key, value = kvp.split("=")
            if key == "n":
                self.assertAlmostEqual(float(value), 17, places=6)
                continue
            if key == "s":
                self.assertAlmostEqual(float(value), 1, places=6)
                continue
            if key == "e":
                self.assertAlmostEqual(float(value), 17, places=6)
                continue
            if key == "w":
                self.assertAlmostEqual(float(value), 1, places=6)
                continue
            if key == "t":
                self.assertAlmostEqual(float(value), 28, places=6)
                continue
            if key == "b":
                self.assertAlmostEqual(float(value), 1, places=6)

    @unittest.skipIf(shutil.which("r.in.pdal") is None, "Cannot find r.in.pdal")
    def test_info_good(self):
        """Validate successful file info printing"""
        out = read_command("r.in.pdal", input=self.las_file, flags="p", quiet=True)
        self.assertIn("File version = 1.4", out)
        self.assertIn("File signature: LASF", out)
        self.assertIn("Point count: 53", out)


if __name__ == "__main__":
    test()
