"""
Name:       r.mode test
Purpose:    Tests r.mode and its flags/options.

Author:     Supreet Singh
SPDX-FileCopyrightText: 2018 Supreet Singh
SPDX-FileCopyrightText: Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class Testrmode(TestCase):
    output = "rmode"
    base = "geology"
    cover = "soils"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.base, flags="d")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule("g.remove", type="raster", flags="f", name=self.output)

    def test_1(self):
        self.assertModule(
            "r.mode", base=self.base, cover=self.cover, output=self.output
        )
        self.assertRasterMinMax(
            map=self.output,
            refmin=21513,
            refmax=46487,
            msg="soils must be between 21513 and 46487",
        )


if __name__ == "__main__":
    test()
