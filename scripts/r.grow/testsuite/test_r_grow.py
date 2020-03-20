"""
Created on Sun Jun 07 22:09:41 2018

@author: Sanjeet Bhatti, Maris Nartiss
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import run_command


class TestRGrow(TestCase):
    """Test r.grow script"""

    mapName = 'lakes'
    mapGrownOutput = 'lakes_grown_100m'
    mapShrunkOutput = 'lakes_shrunk_100m'
    mapNoNULL = 'elevation'
    mapShrunkNoNULL = 'elevation_shrunk'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=(cls.mapGrownOutput,
                            cls.mapShrunkOutput,
                            cls.mapShrunkNoNULL))
        cls.del_temp_region()

    def test_grow(self):
        """Grow test"""
        module = SimpleModule('r.grow', input=self.mapName,
                              output=self.mapGrownOutput,
                              radius=10)
        self.assertModule(module)

    def test_shrink(self):
        """Shrink test"""
        module = SimpleModule('r.grow', input=self.mapName,
                              output=self.mapShrunkOutput,
                              radius=-10)
        self.assertModule(module)

    def test_shrink_null(self):
        """Shrinking of map without NULL values
        Based on https://github.com/OSGeo/grass/pull/343"""
        shrinked_string = '56-156'
        shrinked = SimpleModule('r.grow', input=self.mapNoNULL,
                              output=self.mapShrunkNoNULL,
                              radius=-10)
        self.assertModule(shrinked)

        shrined_range = SimpleModule('r.describe', flags='i', _map=self.mapShrunkNoNULL)
        self.runModule(shrined_range)
        self.assertLooksLike(shrinked_string, str(shrined_range.outputs.stdout).strip())

if __name__ == '__main__':
    test()
