"""Test of r.univar
In multithreading computings, summation between threads may cause floating point errors.
Besides, the summation over the whole map may cause floating point errors.
This test is to check if the output of r.univar is correct.

@author Chung-Yuan Liang, 2025
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestRasterUnivar(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name="map_a,map_b,map_negative,zone_map,zone_map_with_gap",
        )

    def setUp(self):
        """Create input data"""
        self.runModule("g.region", rows=10000, cols=10000, res=1)
        self.runModule(
            "r.mapcalc", expression="map01 = double(1) + double(0.1)", overwrite=True
        )

        # Output of r.univar with single thread
        single_t = SimpleModule(
            "r.univar",
            map="map01",
            flags="g",
            nprocs=1,
        )
        self.assertModule(single_t)
        self.out_single = single_t.outputs.stdout

    def test_threads_2(self):
        # Output of r.univar with 2 threads
        nprocs = 2
        multi_t = SimpleModule(
            "r.univar",
            map="map01",
            flags="g",
            nprocs=nprocs,
        )
        self.assertModule(multi_t)
        out_multi = multi_t.outputs.stdout
        self.assertMultiLineEqual(
            first=self.out_single, second=out_multi, msg=f"{nprocs} threads failed"
        )

    def test_threads_4(self):
        # Output of r.univar with 4 threads
        nprocs = 4
        multi_t = SimpleModule(
            "r.univar",
            map="map01",
            flags="g",
            nprocs=nprocs,
        )
        self.assertModule(multi_t)
        out_multi = multi_t.outputs.stdout
        self.assertMultiLineEqual(
            first=self.out_single, second=out_multi, msg=f"{nprocs} threads failed"
        )

    def test_threads_8(self):
        # Output of r.univar with 8 threads
        nprocs = 8
        multi_t = SimpleModule(
            "r.univar",
            map="map01",
            flags="g",
            nprocs=nprocs,
        )
        self.assertModule(multi_t)
        out_multi = multi_t.outputs.stdout
        self.assertMultiLineEqual(
            first=self.out_single, second=out_multi, msg=f"{nprocs} threads failed"
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
