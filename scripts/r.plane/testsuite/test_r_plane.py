"""
Created on Sun Jun 08 12:12:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRPlane(TestCase):
    """Test r.plane script"""

    mapName = 'elevation'
    mapOutput = 'myplane45'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=cls.mapOutput)
        cls.del_temp_region()

    def test_creates_raster_plane_map(self):
        """Create a tilted plane raster map test"""
        module = SimpleModule('r.plane', output=self.mapOutput, dip=45,
                              easting=527500.0, northing=165000.0,
                              elevation=1000, type='FCELL')
        self.assertModule(module)

        self.assertRasterExists(self.mapOutput)

if __name__ == '__main__':
    test()
