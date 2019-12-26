"""
Created on Sun Jun 08 13:20:31 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRRGB(TestCase):
    """Test r.rgb script"""

    mapName = 'elevation'
    red = 'elevation.r'
    green = 'elevation.g'
    blue = 'elevation.b'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=(cls.red, cls.green, cls.blue))
        cls.del_temp_region()

    def test_rgb_maps(self):
        """Generates rgb maps from a raster map test"""
        module = SimpleModule('r.rgb', input=self.mapName, red=self.red,
                              green=self.green, blue=self.blue)
        self.assertModule(module)

        self.assertRasterExists(self.red)
        self.assertRasterExists(self.green)
        self.assertRasterExists(self.blue)

if __name__ == '__main__':
    test()
