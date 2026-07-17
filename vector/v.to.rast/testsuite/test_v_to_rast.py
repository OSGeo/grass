"""
Name:       v.to.rast test

Purpose:    Test v.to.rast

Author:     Vaclav Petras

SPDX-FileCopyrightText: 2023 Vaclav Petras
SPDX-FileCopyrightText: Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestParameters(TestCase):
    """Test v.to.rast"""

    output = "roads"

    @classmethod
    def setUpClass(cls):
        """Specify region for raster creation for this class"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="roadsmajor", res=10, flags="a")

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove maps after each test method"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[self.output],
        )

    def test_legacy_use_interface(self):
        """Check that a legacy value for use parameter works"""
        self.assertModule(
            "v.to.rast", input="roadsmajor", output=self.output, use="val", value=1
        )

    def test_use_interface(self):
        """Check that use=value value=1 works"""
        self.assertModule(
            "v.to.rast", input="roadsmajor", output=self.output, use="value", value=1
        )
        self.assertRasterFitsInfo(raster=self.output, reference={"min": 1, "max": 1})


if __name__ == "__main__":
    test()
