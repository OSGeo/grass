"""
Name:       v.clip test
Purpose:    Tests v.clip input parsing.
            Uses NC Basic data set.

Author:     Luca Delucchi
Copyright:  (C) 2017 by Luca Delucchi and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

class TestClipling(TestCase):
    inpclip = 'zipcodes'
    inpoint = 'hospitals'
    inlines = 'roadsmajor'
    inpoly = 'geology'
    outreg = 'hospreg'
    outclip = 'hospzip'
    outline = 'roadsgarner'
    outpoly = 'geogarner'
    outdiss = 'geodiss'
    garner = 'garner'

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule('g.region', vector=cls.inpclip)
        cls.runModule('v.extract', input=cls.inpclip, output=cls.garner, 
                      where="ZIPNAME = '{na}'".format(na=cls.garner.upper()))

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule('g.remove', flags='f', type='vector',
                      name=(cls.outclip, cls.outreg, cls.garner, cls.outline,
                            cls.outpoly, cls.outdiss))
        cls.del_temp_region()

    def test_points(self):
        """Test clipping points"""
        self.assertModule('v.clip', input=self.inpoint, clip=self.inpclip,
                          output=self.outclip)
        self.assertVectorExists(self.outclip)
        topology = dict(points=8)
        self.assertVectorFitsTopoInfo(self.outclip, topology)

    def test_region(self):
        """Test clipping point by region"""
        self.assertModule('v.clip', input=self.inpoint, clip=self.inpclip,
                          output=self.outreg, flags='r')
        self.assertVectorExists(self.outreg)
        topology = dict(points=13)
        self.assertVectorFitsTopoInfo(self.outreg, topology)

    def test_lines(self):
        """Test clipping lines"""
        self.assertModule('v.clip', input=self.inlines, clip=self.garner,
                          output=self.outline)
        self.assertVectorExists(self.outline)
        topology = dict(lines=13, nodes=16)
        self.assertVectorFitsTopoInfo(self.outline, topology)

    def test_poly(self):
        """Test clipping polygon without dissolve"""
        self.assertModule('v.clip', input=self.inpoly, clip=self.inpclip,
                          output=self.outpoly)
        self.assertVectorExists(self.outpoly)
        topology = dict(areas=275)
        self.assertVectorFitsTopoInfo(self.outpoly, topology)

    def test_poly_diss(self):
        """Test clipping polygon without dissolve"""
        self.assertModule('v.clip', input=self.inpoly, clip=self.inpclip,
                          output=self.outdiss, flags='d')
        self.assertVectorExists(self.outdiss)
        topology = dict(areas=276)
        self.assertVectorFitsTopoInfo(self.outdiss, topology)

if __name__ == '__main__':
    test()
