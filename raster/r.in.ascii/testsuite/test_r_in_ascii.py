"""
Name:       r.in.ascii test
Purpose:    Tests r.in.ascii and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command

INPUT_NOQUOTES="""north:                   4299000.00
south:                   4247000.00
east:                     528000.00
west:                     500000.00
rows:                         10   
cols:                         15   
null:                      -9999   

1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 """

INPUT_TSV="""north:                   4299000.00
south:                   4247000.00
east:                     528000.00
west:                     500000.00
rows:                         10   
cols:                         15   
null:                      -9999   

1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15
1\ 2\ 3\ 4\ 5\ 6\ 7\ 8\ 9\ 10\ 11\ 12\ 13\ 14\ 15 """

INPUT_UNCOMMON="""north:                   4299000.00
south:                   4247000.00
east:                     528000.00
west:                     500000.00
rows:                         10   
cols:                         15   
null:                      -9999   

1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15
1@ 2@ 3@ 4@ 5@ 6@ 7@ 8@ 9@ 10@ 11@ 12@ 13@ 14@ 15 """

class SimpleCsvTestCase(TestCase):
    ascii_test = 'ascii'

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule('g.region', n=4299000.00, s=4247000.00, e=528000.00, w=500000.00)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        """Remove the raster map after each test method"""
        self.runModule('g.remove', flags='f', type='raster', pattern=self.ascii_test)

    def test_no_text_delimeter(self):
        """Test loading no quotes"""
        self.assertModule('r.in.ascii', input='-', output=self.ascii_test,
                          type='CELL', stdin_=INPUT_NOQUOTES)
        self.assertRasterMinMax(map=self.ascii_test, refmin=1, refmax=15,
	                        msg="ascii_test in degrees must be between 1 and 15")

    def test_text_delimeter(self):
        """Testing with external file"""
        self.assertModule('r.in.ascii', input='data/input_ascii.txt', output=self.ascii_test,
                          type='CELL')
        self.assertRasterMinMax(map=self.ascii_test, refmin=1, refmax=5,
	                        msg="ascii_test in degrees must be between 1 and 5")

    def test_tsv(self):
        """Test loading TSV"""
        self.assertModule('r.in.ascii', input='-', output=self.ascii_test,
                          type='CELL', stdin_=INPUT_TSV)
        self.assertRasterMinMax(map=self.ascii_test, refmin=1, refmax=15,
	                        msg="ascii_test in degrees must be between 1 and 15")

    def test_uncommon_delims(self):
        """Test loading with uncommon delimiters"""
        self.assertModule('r.in.ascii', input='-', output=self.ascii_test,
                          type='CELL', stdin_=INPUT_UNCOMMON)
        self.assertRasterMinMax(map=self.ascii_test, refmin=1, refmax=15,
	                        msg="ascii_test in degrees must be between 1 and 15")

if __name__ == '__main__':
    test()
