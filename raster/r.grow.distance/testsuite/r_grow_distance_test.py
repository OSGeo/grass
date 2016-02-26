"""
Name:      r_grow_distance_test
Purpose:   This script is to demonstrate a unit test for r.grow.distance
           module.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestGrowDistance(TestCase):
    """Test case for grow distance module"""

    # Setup variables to be used for outputs
    distance = 'test_distance'
    lakes = 'lakes'
    elevation = 'elevation'

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and setup"""
        # Always use the computational region of the raster elevation
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.elevation)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created from the grow distance module

        This is executed after each test run.
        """
        self.runModule('g.remove', flags='f', type='raster',
                       name=self.distance)

    def test_grow(self):
        """Test to see if the outputs are created"""
        # run the grow distance module
        self.assertModule('r.grow.distance', input=self.lakes,
                          distance=self.distance)
        # check to see if distance output is in mapset
        self.assertRasterExists(self.distance,
                                msg='distance output was not created')
        self.assertRasterMinMax(self.distance, 0, 5322,
                                msg='distance output not in range')


if __name__ == '__main__':
    test()
