"""
TEST:      r.random.cells

AUTHOR(S): Vaclav Petras

PURPOSE:   Test r.random.cells

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""


from grass.gunittest.case import TestCase
from grass.gunittest.main import test



class TestCounts(TestCase):

    # TODO: replace by unified handing of maps
    to_remove = []
    all_rast = "r_random_cells_all"
    some_rast = "r_random_cells_some"
    count_rast = "r_random_cells_count"
    n_cells = 50

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # (20-10) / 2 * (60-40) / 2 = 50 cells
        cls.runModule('g.region', n=20, s=10, e=60, w=40, res=2)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule('g.remove', flags='f', type='raster',
                          name=','.join(cls.to_remove), verbose=True)

    def test_fill_all(self):
        self.assertModule('r.random.cells', output=self.all_rast,
                          distance=0.01, seed=100)
        self.to_remove.append(self.all_rast)
        self.assertRasterFitsUnivar(
            self.all_rast,
            reference=dict(cells=self.n_cells, n=self.n_cells, null_cells=0,
                           min=1, max=self.n_cells))

    def test_fill_some(self):
        self.assertModule('r.random.cells', output=self.some_rast,
                          distance=2, seed=100)
        self.to_remove.append(self.some_rast)
        self.assertRasterFitsUnivar(
            self.some_rast,
            reference=dict(cells=self.n_cells, min=1))
        # it is hard to say how much but it will be less than half
        self.assertRasterMinMax(self.some_rast, 1, self.n_cells / 2)

    def test_fill_count(self):
        count = 12
        self.assertModule('r.random.cells', output=self.count_rast,
                          distance=2, seed=100, ncells=count)
        self.to_remove.append(self.count_rast)
        self.assertRasterFitsUnivar(
            self.count_rast,
            reference=dict(cells=self.n_cells, n=count,
                           null_cells=self.n_cells - count,
                           min=1, max=count))
        # it is hard to say how much but it will be less than half
        self.assertRasterMinMax(self.count_rast, 1, count)


if __name__ == '__main__':
    test()
