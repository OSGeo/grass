"""Test of g.region

@author Anna Petrasova
"""
<<<<<<< HEAD
<<<<<<< HEAD

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import call_module
=======
from grass.gunittest.case import TestCase
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
from grass.gunittest.case import TestCase
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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

<<<<<<< HEAD
<<<<<<< HEAD
    def test_f_flag(self):
        line = call_module("g.region", flags="fglecn3", capture_stdout=True)
        self.assertEqual(1, len(line.splitlines()))

=======
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
