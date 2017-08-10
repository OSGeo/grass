from grass.gunittest.case import TestCase
from grass.gunittest.main import test


# TODO: add more expressions
# TODO: add tests with prepared data

class TestBasicOperations(TestCase):

    # TODO: replace by unified handing of maps
    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region',
            n=85, s=5, e=85, w=5,
            b=0, t=2000,
            res=1, res3=1)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule('g.remove', flags='f', type='raster_3d',
                name=','.join(cls.to_remove), verbose=True)

    def test_difference_of_the_same_map_double(self):
        """Test zero difference of map with itself"""
        self.runModule('r3.mapcalc', flags='s',
                       expression='a = rand(1.0, 200)')
        self.to_remove.append('a')
        self.assertModule('r3.mapcalc',
            expression='diff_a_a = a - a')
        self.to_remove.append('diff_a_a')
        self.assertRaster3dMinMax('diff_a_a', refmin=0, refmax=0)

    def test_difference_of_the_same_map_float(self):
        """Test zero difference of map with itself"""
        self.runModule('r3.mapcalc', flags='s',
                       expression='af = rand(float(1), 200)')
        self.to_remove.append('af')
        self.assertModule('r3.mapcalc',
            expression='diff_af_af = af - af')
        self.to_remove.append('diff_af_af')
        self.assertRaster3dMinMax('diff_af_af', refmin=0, refmax=0)

    def test_difference_of_the_same_expression(self):
        """Test zero difference of two same expressions"""
        self.assertModule('r3.mapcalc',
            expression='diff_e_e = 3 * x() * y() * z() - 3 * x() * y() * z()')
        self.to_remove.append('diff_e_e')
        self.assertRaster3dMinMax('diff_e_e', refmin=0, refmax=0)

    def test_nrows_ncols_ndepths_sum(self):
        """Test if sum of nrows, ncols and ndepths matches one
        expected from current region settigs"""
        self.assertModule('r3.mapcalc',
            expression='nrows_ncols_ndepths_sum = nrows() + ncols() + ndepths()')
        self.to_remove.append('nrows_ncols_ndepths_sum')
        self.assertRaster3dMinMax('nrows_ncols_ndepths_sum', refmin=2160, refmax=2160)


if __name__ == '__main__':
    test()
