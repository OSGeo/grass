"""
Created on Fri 20 Nov 2020

@author: Markus Neteler
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

import os


class TestDPolar(TestCase):
    """Test d.polar script"""

    mapName = 'aspect'
    epsFile = 'polar.eps'

    @classmethod
    def setUpClass(cls):
        """Set region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

        if (os.path.isfile(cls.epsFile)):
            os.remove(cls.epsFile)

    def test_d_polar(self):
        """EPS file test"""
        module = SimpleModule('d.polar', map=self.mapName,
                              output=self.epsFile)
        self.assertModule(module)

        self.assertFileExists(filename=self.epsFile)

if __name__ == '__main__':
    test()
