"""
Name:       r.reclass test
Purpose:    Tests r.reclass and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

rules1 = """
1 2 3   = 1    good quality
4 5     = 2    poor quality
"""

rules2="""
1 3 5   = 1    poor quality
2 4 6   = 2    good quality
*       = NULL
"""

rules3="""
1 thru 10	= 1
11 thru 20	= 2
21 thru 30	= 3
30 thru 40      = NULL
"""

class Testrr(TestCase):
    output='reclass'
    input='elevation_shade'
 
    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)
	
    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        self.runModule('g.remove', type='raster', flags='f', name=self.output)

    def test_rules1(self):
        """Testing rules 1 """
        reclass = SimpleModule('r.reclass', input=self.input, output=self.output,
                              rules='-')
        reclass.inputs.stdin = rules1
        self.assertModule(reclass)
        info = 'min=1\nmax=2\ndatatype=CELL'
        self.assertRasterFitsInfo(raster='reclass', reference=info)

    def test_rules2(self):
        """Testing Rules2"""
        reclass = SimpleModule('r.reclass', input=self.input, output=self.output,
                              rules='-')
        reclass.inputs.stdin = rules2
        self.assertModule(reclass)
        info = 'min=1\nmax=2\ndatatype=CELL'
        self.assertRasterFitsInfo(raster='reclass', reference=info)

    def test_rules3(self):
        """Testing rules3"""
        reclass = SimpleModule('r.reclass', input=self.input, output=self.output,
                              rules='-')
        reclass.inputs.stdin = rules3
        self.assertModule(reclass)
        info = 'min=1\nmax=3\ndatatype=CELL'
        self.assertRasterFitsInfo(raster='reclass', reference=info)

    def test_rules4(self):
        """Testing rules with external file"""
        reclass = SimpleModule('r.reclass', input=self.input, output=self.output,
                              rules='data/rules.txt')
        self.assertModule(reclass)
        info = 'min=1\nmax=3\ndatatype=CELL'
        self.assertRasterFitsInfo(raster='reclass', reference=info)

if __name__ == '__main__':
    test()
