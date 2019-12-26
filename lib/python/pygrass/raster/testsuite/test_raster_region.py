# -*- coding: utf-8
import six
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.raster import RasterRow
from grass.pygrass.raster import raster2numpy
from grass.pygrass.gis.region import Region


class RasterRowRegionTestCase(TestCase):

    name = "RasterRowRegionTestCase_map"

    @classmethod
    def setUpClass(cls):
        """Create test raster map and region"""
        cls.use_temp_region()
        cls.runModule("g.region", n=40, s=0, e=40, w=0, res=10)
        cls.runModule("r.mapcalc", expression="%s = row() + (10.0 * col())" % (cls.name),
            overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exist"""
        cls.runModule("g.remove", flags='f', type='raster',
                      name=cls.name)
        cls.del_temp_region()

    def test_resampling_1(self):
        
        region = Region()
        
        region.ewres = 4
        region.nsres = 4
        region.north = 30
        region.south = 10
        region.east = 30
        region.west = 10
        region.adjust(rows=True, cols=True)
        
        rast = RasterRow(self.name)
        rast.set_region(region)
        rast.open(mode='r')
        
        six.assertCountEqual(self, rast[0].tolist(), [22, 22, 22, 22, 22, 32, 32, 32, 32, 32])
        six.assertCountEqual(self, rast[5].tolist(), [23, 23, 23, 23, 23, 33, 33, 33, 33, 33])
        
        rast.close()

    def test_resampling_2(self):
        
        region = Region()
        
        region.ewres = 5
        region.nsres = 5
        region.north = 60
        region.south = -20
        region.east = 60
        region.west = -20
        region.adjust(rows=True, cols=True)
        
        rast = RasterRow(self.name)
        rast.set_region(region)
        rast.open(mode='r')
        
        """
        [nan, nan, nan, nan, nan, nan, nan, nan]
        [nan, nan, nan, nan, nan, nan, nan, nan]
        [nan, nan, 11.0, 21.0, 31.0, 41.0, nan, nan]
        [nan, nan, 12.0, 22.0, 32.0, 42.0, nan, nan]
        [nan, nan, 13.0, 23.0, 33.0, 43.0, nan, nan]
        [nan, nan, 14.0, 24.0, 34.0, 44.0, nan, nan]
        [nan, nan, nan, nan, nan, nan, nan, nan]
        [nan, nan, nan, nan, nan, nan, nan, nan]
        """

        six.assertCountEqual(self, rast[2].tolist()[2:6], [11., 21., 31., 41.])
        six.assertCountEqual(self, rast[5].tolist()[2:6], [14., 24., 34., 44.])
        
        rast.close()

    def test_resampling_to_numpy(self):
        
        region = Region()
        region.ewres = 0.1
        region.nsres = 0.1
        region.adjust()
        region.set_raster_region()
        
        a = raster2numpy(self.name)
        
        self.assertEqual(len(a), 400)
        
        region.ewres = 1
        region.nsres = 1
        region.adjust()
        region.set_raster_region()
        
        a = raster2numpy(self.name)
        
        self.assertEqual(len(a), 40)

        region.ewres = 5
        region.nsres = 5
        region.adjust()
        region.set_raster_region()
        
        a = raster2numpy(self.name)
        
        self.assertEqual(len(a), 8)

if __name__ == '__main__':
    test()
