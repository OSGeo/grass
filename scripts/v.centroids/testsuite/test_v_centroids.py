"""
Created on Thrs Jun 09 11:26:12 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVCentroids(TestCase):
    """Test v.centroids script"""
    mapName = 'busroute11'
    outRouteMap = 'busroute11_boundary'
    fromType = 'line'
    toType = 'boundary'
    outAreaMap = 'busroute11_area'

    @classmethod
    def setUpClass(cls):
        """Create an area from a closed line"""
        cls.runModule('v.type', input=cls.mapName, output=cls.outRouteMap,
                      from_type=cls.fromType, to_type=cls.toType)

    @classmethod
    def tearDownClass(cls):
        """Remove the generated maps"""
        cls.runModule('g.remove', flags='f', type='vector',
                      name=(cls.outRouteMap, cls.outAreaMap))

    def test_area(self):
        """Adds missing centroids to closed boundaries test"""
        module = SimpleModule('v.centroids', input=self.outRouteMap,
                              output=self.outAreaMap)
        self.assertModule(module)
        self.assertVectorExists(self.outAreaMap)

if __name__ == '__main__':
    test()
