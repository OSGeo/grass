"""
Name:       r.reclass.area test
Purpose:    Tests r.reclass.area and its flags/options.

Author:     Sunveer Singh, Google Code-in 2018
SPDX-FileCopyrightText: 2018 Sunveer Singh
SPDX-FileCopyrightText: Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class Testrr(TestCase):
    input = "zipcodes"
    output = "rraoutput"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule("g.remove", type="raster", flags="f", name=self.output)

    def test_flag_c(self):
        """Testing flag c"""
        string = """min=27603
        max=27607
        cells=2025000"""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=self.output,
            value=2000,
            mode="greater",
            flags="c",
        )
        self.assertRasterFitsUnivar(self.output, reference=string, precision=2)

    def test_flag_d(self):
        """Testing flag d"""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=self.output,
            value=2000,
            mode="lesser",
            flags="d",
        )
        self.assertRasterMinMax(
            map=self.output,
            refmin=27511,
            refmax=27610,
            msg="Output Map in degrees must be between 27511 and 27610",
        )

    def test_module_output(self):
        """Testing Module without flags"""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=self.output,
            value=2000,
            mode="greater",
        )
        self.assertRasterMinMax(
            map=self.output,
            refmin=27603,
            refmax=27607,
            msg="Output Map in degrees must be between 27603 and 27607",
        )

    def test_method_rmarea(self):
        """Testing Module without flags"""
        self.assertModule(
            "r.reclass.area",
            input=self.input,
            output=self.output,
            value=2000,
            mode="lesser",
            method="rmarea",
        )
        self.assertRasterMinMax(
            map=self.output,
            refmin=27511,
            refmax=27610,
            msg="Output Map in degrees must be between 27511 and 27610",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
