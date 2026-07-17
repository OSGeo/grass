"""
Name:       r.to.vect test
Purpose:    Tests r.to.vect and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
SPDX-FileCopyrightText: 2017 Sunveer Singh
SPDX-FileCopyrightText: Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class Testrr(TestCase):
    input = "lakes"
    output = "testrtovect"
    point = "point"
    area = "area"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule("g.remove", type="vector", flags="f", name=self.output)

    def test_flags(self):
        """Testing flag s"""
        self.assertModule(
            "r.to.vect",
            input=self.input,
            output=self.output,
            type=self.point,
            flags="s",
        )
        topology = {"points": 36011, "lines": 0, "areas": 0}
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_flagz(self):
        """Testing flag z"""
        self.assertModule(
            "r.to.vect",
            input=self.input,
            output=self.output,
            type=self.point,
            flags="z",
        )
        topology = {"points": 36011, "lines": 0, "areas": 0}
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_flagb(self):
        """Testing flag b"""
        self.assertModule(
            "r.to.vect", input=self.input, output=self.output, type=self.area, flags="b"
        )
        topology = {"points": 0, "lines": 0, "areas": 0}
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_flagt(self):
        """Testing flag t"""
        self.assertModule(
            "r.to.vect", input=self.input, output=self.output, type=self.area, flags="t"
        )
        topology = {"points": 0, "lines": 0, "areas": 33}
        self.assertVectorFitsTopoInfo(self.output, topology)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
