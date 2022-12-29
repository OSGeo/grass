"""
Created on Sun Jun 07 21:01:34 2018

@author: Sanjeet Bhatti
@author: Vaclav Petras
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestDbUnivar(TestCase):
    """Test running db.univar script with NULL values and extended stats"""

    column_name = "heights"
    map_name = "samples"

    @classmethod
    def setUpClass(cls):  # pylint: disable=invalid-name
        """Generate vector points in extend larger than raster with values"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")
        cls.runModule("g.region", e="e+5000", w="w-5000", s="s-5000", n="n+5000")
        cls.runModule("v.random", output=cls.map_name, npoints=100, seed=42)
        cls.runModule(
            "v.db.addtable",
            map=cls.map_name,
            columns="{} double precision".format(cls.column_name),
        )
        cls.runModule(
            "v.what.rast", map=cls.map_name, raster="elevation", column=cls.column_name
        )

    @classmethod
    def tearDownClass(cls):  # pylint: disable=invalid-name
        """Remove temporary region and vector"""
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="vector", name=cls.map_name)

    def test_calculate(self):
        """Check that db.univar runs"""
        module = SimpleModule("db.univar", table=self.map_name, column=self.column_name)
        self.assertModule(module)

    def test_calculate_extended(self):
        """Check that db.univar -e runs"""
        module = SimpleModule(
            "db.univar", table=self.map_name, flags="e", column=self.column_name
        )
        self.assertModule(module)


if __name__ == "__main__":
    test()
