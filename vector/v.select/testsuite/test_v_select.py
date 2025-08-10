"""
Name:       v.select test
Purpose:    Tests v.select and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase


class TestRasterReport(TestCase):
    binput = "zipcodes"
    ainput = "geology"
    output = "testvselect"

    def tearDown(self):
        self.runModule("g.remove", type="vector", flags="f", name=self.output)

    def test_opo(self):
        """Testing operator overlap"""
        self.assertModule(
            "v.select",
            ainput=self.ainput,
            binput=self.binput,
            output=self.output,
            operator="overlap",
        )
        topology = {"areas": 97}
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_opd(self):
        """Testign operator disjoint"""
        self.assertModule(
            "v.select",
            ainput=self.ainput,
            binput=self.binput,
            output=self.output,
            operator="disjoint",
        )
        topology = {"areas": 1770}
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_ope(self):
        """Testing operator equals"""
        self.assertModule(
            "v.select",
            ainput=self.ainput,
            binput=self.binput,
            output=self.output,
            operator="equals",
        )
        topology = {"areas": 0}
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_opt(self):
        """Testing operator touches"""
        self.assertModule(
            "v.select",
            ainput=self.ainput,
            binput=self.binput,
            output=self.output,
            operator="touches",
        )
        topology = {"areas": 0}
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_opw(self):
        """Testing operator within"""
        self.assertModule(
            "v.select",
            ainput=self.ainput,
            binput=self.binput,
            output=self.output,
            operator="within",
        )
        topology = {"areas": 17}
        self.assertVectorFitsTopoInfo(self.output, topology)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
