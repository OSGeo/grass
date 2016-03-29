"""Test of r.univar

@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

class TestRastStats(TestCase):

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        cls.del_temp_region()

    def tearDown(self):
        self.runModule("g.remove", flags='f', type="raster", name="map_a")
        self.runModule("g.remove", flags='f', type="raster", name="map_b")
        self.runModule("g.remove", flags='f', type="raster", name="zone_map")

    def setUp(self):
        """Create input data
        """
        self.runModule("g.region", res=1, n=90, s=0, w=0, e=90)
        self.runModule("r.mapcalc", expression="map_a = 100 + row() + col()",
                       overwrite=True)
        self.runModule("r.mapcalc", expression="zone_map = if(row() < 20, 1,2)",
                       overwrite=True)
        self.runModule("r.to.vect", input="zone_map", output="zone_map",
                       type="area", overwrite=True)

    def test_1(self):
        # Output of v.rast.stats
        univar_string="""cat|value|label|a_minimum|a_maximum|a_sum
1|1||102|209|265905
2|2||121|280|1281195
"""
        
        self.assertModule("v.rast.stats",  map="zone_map", raster="map_a",
                          method=["minimum","maximum","sum"], flags="c",
                          column_prefix="a")
        v_db_select = SimpleModule("v.db.select", map="zone_map")
        
        self.runModule(v_db_select)        
        self.assertLooksLike(univar_string, v_db_select.outputs.stdout)

class TestRastStatsFails(TestCase):

    def test_error_handling_a(self):
        # No vector map
        self.assertModuleFail("v.rast.stats", raster="map_a",
                              column_prefix="a")

    def test_error_handling_b(self):
        # No raster map
        self.assertModuleFail("v.rast.stats", map="zone_map",
                              column_prefix="a")
        
    def test_error_handling_d(self):
        # No column_prefix
        self.assertModuleFail("v.rast.stats", map="zone_map", raster="map_b")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()


