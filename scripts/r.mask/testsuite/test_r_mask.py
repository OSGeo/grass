"""
Created on Sun Jun 07 22:19:41 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRMask(TestCase):
    """Test r.mask script"""

    mapName = 'lakes'
    mapNameOther = 'elevation'
    values = 'min=56.8785\nmax=134.87'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region. Remove mask"""
        cls.del_temp_region()

        cls.runModule('r.mask', flags='r')

    def test_mask(self):
        """Mask test"""
        module = SimpleModule('r.mask', raster=self.mapName)
        self.assertModule(module)

        self.assertRasterFitsUnivar(raster=self.mapNameOther,
                                    reference=self.values,
                                    precision=5)

if __name__ == '__main__':
    test()
