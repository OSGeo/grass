"""
Name:       r.tile test
Purpose:    Tests r.tile and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2018
Copyright:  (C) 2018 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

class Testrr(TestCase):
    input='elevation'
    output='tile'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)
	
    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()


    def test_basic(self):
        """Running a basic test"""
        self.assertModule('r.tile', input=self.input, output=self.output, width=1500/2, height=1350/2)

    def test_univar(self):
        """Testing the output map tile-001-002"""
        string="""min=55.5787925720215
        n=506250
        max=144.267288208008"""
        self.assertModule('r.tile', input=self.input, output=self.output, width=1500/2, height=1350/2)
        self.assertRasterFitsUnivar('tile-000-001',
	                            reference=string, precision=2)

    def test_overlap(self):
        """Testing overlap parameter with output map tile-000-000"""
        tile="tile-000-000"
        self.assertModule('r.tile', input=self.input, output=self.output, width=1500/2, height=1350/2, overlap=250)
        self.assertRasterMinMax(map=tile, refmin=74.75374, refmax=156.3299,
	                        msg="tile-000-000 in degrees must be between 74.75374 and 156.3299") 

    def test_minmax(self):
        """Testing output map tile-000-001"""
        tile1="tile-000-001"
        self.assertModule('r.tile', input=self.input, output=self.output, width=1500/2, height=1350/2)
        self.assertRasterMinMax(map=tile1, refmin=55.57879, refmax=144.2673,
	                        msg="tile-000-001 in degrees must be between 55.57879 and 144.2673")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()

