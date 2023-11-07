"""Test of g.region

@author Anna Petrasova
"""
from grass.gunittest.case import TestCase
import grass.script as gs


class TestRegion(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def test_d_flag(self):
        n = 228500
        res = 1
        n_default = 320000
        res_default = 500

        self.runModule("g.region", res=res, n=n)
        region = gs.region()
        self.assertEqual(n, region["n"])
        self.assertEqual(res, region["nsres"])

        # test default with no update flag
        self.runModule("g.region", flags="dup")
        region = gs.region()
        self.assertEqual(n, region["n"])
        self.assertEqual(res, region["nsres"])

        # test set default
        self.runModule("g.region", flags="d")
        region = gs.region()
        self.assertEqual(n_default, region["n"])
        self.assertEqual(res_default, region["nsres"])


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
