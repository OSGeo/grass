"""
Created on Sun Jun 08 19:08:07 2018

@author: Sanjeet Bhatti
@author: Vaclav Petras
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVDbUnivar(TestCase):
    """Test v.db.univar script"""

    column_name = "heights"
    map_name = "samples"

    @classmethod
    def setUpClass(cls):
        """Generate vector points with values from raster"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")
        cls.runModule("v.random", output=cls.map_name, npoints=100, seed=42)
        cls.runModule(
            "v.db.addtable",
            map=cls.map_name,
            columns=f"{cls.column_name} double precision",
        )
        cls.runModule(
            "v.what.rast", map=cls.map_name, raster="elevation", column=cls.column_name
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region and vector"""
        cls.runModule("g.remove", flags="f", type="vector", name=cls.map_name)
        cls.del_temp_region()

    def test_calculate(self):
        """v.db.univar runs"""
        module = SimpleModule("v.db.univar", map=self.map_name, column=self.column_name)
        self.assertModule(module)

    def test_calculate_extended(self):
        """v.db.univar -e runs"""
        module = SimpleModule(
            "v.db.univar", map=self.map_name, flags="e", column=self.column_name
        )
        self.assertModule(module)


if __name__ == "__main__":
    test()
