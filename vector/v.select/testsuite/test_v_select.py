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
    binput = "bridges"
    ainput = "geology"
    output = "testvselect"
    overlap = "geonames_wake"
    disjoint = "schools_wake"
    equals = "streets_wake"
    touches = "zipcodes_wake"
    within = "geonames_wake"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(cls):
        cls.runModule('g.remove', type='vector', flags='f', name=cls.output)

    def test_opo(self):
        """Testing operator overlap"""
        self.assertModule('v.select', ainput=self.ainput, binput=self.binput,
                          output=self.output, operator='overlap')
        topology = dict(points=1088, lines=0, areas=0)
        self.assertVectorFitsTopoInfo(self.overlap, topology)

    def test_opd(self):
        """Testign operator disjoint """
        self.assertModule('v.select', ainput=self.ainput, binput=self.binput,
                          output=self.output, operator='disjoint')
        topology = dict(points=167, lines=0, areas=0)
        self.assertVectorFitsTopoInfo(self.disjoint, topology)

    def test_ope(self):
        """Testing operator equals """
        self.assertModule('v.select', ainput=self.ainput, binput=self.binput,
                          output=self.output, operator='equals')
        topology = dict(points=0, lines=49746, areas=0)
        self.assertVectorFitsTopoInfo(self.equals, topology)

    def test_opt(self):
        """Testing operator touches"""
        self.assertModule('v.select', ainput=self.ainput, binput=self.binput,
                          output=self.output, operator='touches')
        topology = dict(points=0, lines=0, areas=48)
        self.assertVectorFitsTopoInfo(self.touches, topology)

    def test_opw(self):
        """Testing operator within"""
        self.assertModule('v.select', ainput=self.ainput, binput=self.binput,
                          output=self.output, operator='within')
        topology = dict(points=1088, lines=0, areas=0)
        self.assertVectorFitsTopoInfo(self.within, topology)


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
