from grass.gunittest.case import TestCase
from grass.gunittest.main import test

THREADS = 4


class TestBasicOperations(TestCase):
    # TODO: replace by unified handing of maps
    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", n=20, s=10, e=25, w=15, res=1)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
                verbose=True,
            )

    def test_difference_of_the_same_map_double(self):
        """Test zero difference of map with itself"""
        self.runModule("r.mapcalc", expression="a = rand(1.0, 200)")
        self.to_remove.append("a")
        self.assertModule("r.mapcalc", expression="diff_a_a = a - a", nprocs=THREADS)
        self.to_remove.append("diff_a_a")
        self.assertRasterMinMax("diff_a_a", refmin=0, refmax=0)

    def test_difference_of_the_same_map_float(self):
        """Test zero difference of map with itself"""
        self.runModule("r.mapcalc", expression="af = rand(float(1), 200)")
        self.to_remove.append("af")
        self.assertModule(
            "r.mapcalc", expression="diff_af_af = af - af", nprocs=THREADS
        )
        self.to_remove.append("diff_af_af")
        self.assertRasterMinMax("diff_af_af", refmin=0, refmax=0)

    def test_difference_of_the_same_map_int(self):
        """Test zero difference of map with itself"""
        self.runModule("r.mapcalc", expression="ai = rand(1, 200)")
        self.to_remove.append("ai")
        self.assertModule(
            "r.mapcalc", expression="diff_ai_ai = ai - ai", nprocs=THREADS
        )
        self.to_remove.append("diff_ai_ai")
        self.assertRasterMinMax("diff_ai_ai", refmin=0, refmax=0)

    def test_difference_of_the_same_expression(self):
        """Test zero difference of two same expressions"""
        self.assertModule(
            "r.mapcalc",
            expression="diff_e_e = 3 * x() * y() - 3 * x() * y()",
            nprocs=THREADS,
        )
        self.to_remove.append("diff_e_e")
        self.assertRasterMinMax("diff_e_e", refmin=0, refmax=0)

    def test_nrows_ncols_sum(self):
        """Test if sum of nrows and ncols matches one
        expected from current region settings"""
        self.assertModule(
            "r.mapcalc",
            expression="nrows_ncols_sum = nrows() + ncols()",
            nprocs=THREADS,
        )
        self.to_remove.append("nrows_ncols_sum")
        self.assertRasterMinMax("nrows_ncols_sum", refmin=20, refmax=20)


class TestRegionOperations(TestCase):
    # TODO: replace by unified handing of maps
    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", n=30, s=15, e=30, w=15, res=5)
        cls.runModule(
            "r.mapcalc", expression="test_region_1 = 1", seed=1, nprocs=THREADS
        )
        cls.runModule("g.region", n=25, s=10, e=25, w=10, res=5)
        cls.runModule(
            "r.mapcalc", expression="test_region_2 = 2", seed=1, nprocs=THREADS
        )
        cls.runModule("g.region", n=20, s=5, e=20, w=5, res=1)
        cls.runModule(
            "r.mapcalc", expression="test_region_3 = 3", seed=1, nprocs=THREADS
        )

        cls.to_remove.append("test_region_1")
        cls.to_remove.append("test_region_2")
        cls.to_remove.append("test_region_3")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
                verbose=True,
            )

    def test_union(self):
        """Test the union region option"""
        self.assertModule(
            "r.mapcalc",
            region="union",
            seed=1,
            expression="test_region_4 = test_region_1 + test_region_2 + test_region_3",
            nprocs=THREADS,
        )
        self.to_remove.append("test_region_4")

        self.assertModuleKeyValue(
            "r.info",
            map="test_region_4",
            flags="gr",
            reference={
                "min": 6,
                "max": 6,
                "cells": 625,
                "north": 30,
                "south": 5,
                "west": 5,
                "east": 30,
                "nsres": 1,
                "ewres": 1,
            },
            precision=0.01,
            sep="=",
        )

    def test_intersect(self):
        """Test the intersect region option"""
        self.assertModule(
            "r.mapcalc",
            region="intersect",
            seed=1,
            expression="test_region_5 = test_region_1 + test_region_2 + test_region_3",
            nprocs=THREADS,
        )
        self.to_remove.append("test_region_5")

        self.assertModuleKeyValue(
            "r.info",
            map="test_region_5",
            flags="gr",
            reference={
                "min": 6,
                "max": 6,
                "cells": 25,
                "north": 20,
                "south": 15,
                "west": 15,
                "east": 20,
                "nsres": 1,
                "ewres": 1,
            },
            precision=0.01,
            sep="=",
        )


if __name__ == "__main__":
    test()
