"""
Created on Sun Jun 08 13:44:07 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRShade(TestCase):
    """Test r.shade script"""

    mapName = 'aspect'
    color = 'elevation'
    outputMap = 'elevation_aspect_shaded'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=cls.outputMap)
        cls.del_temp_region()

    def test_shade_map(self):
        """Generates a color raster map over shaded relief map test"""
        module = SimpleModule('r.shade', shade=self.mapName, color=self.color,
                              output=self.outputMap)
        self.assertModule(module)

        self.assertRasterExists(self.outputMap)

if __name__ == '__main__':
    test()
