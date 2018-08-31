"""
Name:       r.info test
Purpose:    Tests r.info and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
class TestReport(TestCase):
    
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        
    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def test_flagg(self):
        """Testing flag g with map geology_30m using simple module"""
        output_str="""north=228500
        south=215000
        east=645000
        west=630000
        nsres=10
        ewres=10
        rows=1350
        cols=1500
        cells=2025000
        datatype=CELL
        ncats=43600"""
        self.assertModuleKeyValue(module='r.info', map='lakes', flags='g', reference=output_str,
	                          precision=2, sep="=")

    def test_flagr(self):
        """Testing flag r with map landcover_1m using simple module"""
        output_str="""min=34300
        max=43600"""
        self.assertModuleKeyValue(module='r.info', map='lakes', flags='r', reference=output_str,
	                          precision=2, sep="=")

    def test_flage(self):
        """Testing flag e with map lsat7_2002_50"""
        self.assertModule('r.info', map='lakes', flags='e')

    def test_flagh(self):
        """Testing flag h with map zipcodes"""
        self.assertModule('r.info', map='lakes', flags='h') 

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()    
