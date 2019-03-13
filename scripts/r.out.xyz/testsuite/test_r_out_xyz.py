"""
Created on Sun Jun 08 10:11:18 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

import os


class TestROutXyz(TestCase):
    """Test r.out.xyz script"""

    mapName = 'elev_lid792_1m'
    csvFile = 'elev_lid792_1m.csv'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

        if (os.path.isfile(cls.csvFile)):
            os.remove(cls.csvFile)

    def test_r_out_xyz(self):
        """ASCII text file test"""
        module = SimpleModule('r.out.xyz', input=self.mapName,
                              output=self.csvFile, separator=",")
        self.assertModule(module)

        self.assertFileExists(filename=self.csvFile)

if __name__ == '__main__':
    test()
