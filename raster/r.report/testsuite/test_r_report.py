"""
Name:       r.report test
Purpose:    Tests r.report and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

import os
from grass.gunittest.case import TestCase


class TestRasterreport(TestCase):
    outfile = 'test_out.csv'


    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()


    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        cls.del_temp_region()
        if os.path.isfile(cls.outfile):
            os.remove(cls.outfile)


    def test_flage(self):
        """Testing flag 'e' with map elevation"""
        self.assertModule('r.report', map='elevation', flags='e')


    def test_flagc(self):
        """Testing flag 'c' with map aspect"""
        self.assertModule('r.report', map='aspect', flags='c')


    def test_flagf(self):
        """Testing flag 'f' with map lakes"""
        self.assertModule('r.report', map='lakes', flags='f')


    def test_flagh(self):
        """Testing flag 'h' with map slope"""
        self.assertModule('r.report', map='slope', flags='h')


    def test_flagn(self):
        """Testing flag 'n' with map urban"""
        self.assertModule('r.report', map='urban', flags='n')


    def test_flaga(self):
        """Testing flag 'a' with map zipcodes"""
        self.assertModule('r.report', map='zipcodes', flags='a')


    def test_output(self):
        """Checking file existence"""
        self.assertModule('r.report', map='lakes', output=self.outfile)
        self.assertFileExists(self.outfile)


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
