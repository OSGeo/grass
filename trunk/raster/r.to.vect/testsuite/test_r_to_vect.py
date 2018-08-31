"""
Name:       r.to.vect test
Purpose:    Tests r.to.vect and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class Testrr(TestCase):
    input = 'lakes'
    output = 'testrtovect'
    point = 'point'
    area = "area"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(cls):
        cls.runModule('g.remove', type='vector', flags='f', name=cls.output)

    def test_flags(self):
        """Testing flag s"""
        self.assertModule('r.to.vect', input=self.input, output=self.output, type=self.point, flags='s')
        topology = dict(points=36011, lines=0, areas=0)
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_flagz(self):
        """Testing flag z"""
        self.assertModule('r.to.vect', input=self.input, output=self.output, type=self.point, flags='z')
        topology = dict(points=36011, lines=0, areas=0)
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_flagb(self):
        """Testing flag b"""
        self.assertModule('r.to.vect', input=self.input, output=self.output, type=self.area, flags='b')
        topology = dict(points=0, lines=0, areas=0)
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_flagt(self):
        """Testing flag t"""
        self.assertModule('r.to.vect', input=self.input, output=self.output, type=self.area, flags='t')
        topology = dict(points=0, lines=0, areas=33)
        self.assertVectorFitsTopoInfo(self.output, topology)


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
