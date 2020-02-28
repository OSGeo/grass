"""Test of r.univar

@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.pygrass.vector import VectorTopo
from grass.pygrass.vector.geometry import Line


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
        self.runModule("g.remove", flags='f', type="raster", name="test_line")

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
        cols = [(u'cat', 'INTEGER PRIMARY KEY'), (u'name', 'VARCHAR(20)')]
        vt = VectorTopo('test_line')
        vt.open('w', tab_cols=cols)
        line1 = Line([(1, 1), (2, 1), (2, 2)])
        line2 = Line([(10, 20), (15, 22), (20, 32), (30, 40)])
        vt.write(line1, ('first',))
        vt.write(line2, ('second',))
        vt.table.conn.commit()
        vt.close()
        

    def test_1(self):
        # Output of v.rast.stats
        univar_string = """cat|value|label|a_minimum|a_maximum|a_sum
1|1||102|209|265905
2|2||121|280|1281195
"""

        self.assertModule("v.rast.stats", map="zone_map", raster="map_a",
                          method=["minimum", "maximum", "sum"], flags="c",
                          column_prefix="a")
        v_db_select = SimpleModule("v.db.select", map="zone_map")

        self.runModule(v_db_select)
        self.assertLooksLike(univar_string, str(v_db_select.outputs.stdout))


    def test_line_d(self):
        output_str = """cat|name|a_median|a_number|a_range
1|first|192|3|1
2|second|181|41|6
"""
        self.assertModule("v.rast.stats", map="test_line", raster="map_a",
                          method=["median", "number", "range"], flags="dc",
                          column_prefix="a")
        v_db_select = SimpleModule("v.db.select", map="test_line")
        
        self.runModule(v_db_select)
        self.assertLooksLike(output_str, str(v_db_select.outputs.stdout))


    def test_line(self):
        output_str = """cat|name|a_median|a_number|a_range
1|first|192|5|2
2|second|181|27|5

"""
        self.assertModule("v.rast.stats", map="test_line", raster="map_a",
                          method=["median", "number", "range"], flags="c",
                          column_prefix="a")
        v_db_select = SimpleModule("v.db.select", map="test_line")
        
        self.runModule(v_db_select)
        self.assertLooksLike(output_str, str(v_db_select.outputs.stdout))


    def test_zone_all(self):
        # Output of v.rast.stats
        univar_string = """cat|value|label|a_number|a_null_cells|a_minimum|a_maximum|a_range|a_average|a_stddev|a_variance|a_coeff_var|a_sum|a_first_quartile|a_median|a_third_quartile|a_percentile_90
1|1||1710|0|102|209|107|155.5|26.5502667908755|704.916666666667|17.0741265536177|265905|133|155.5|178|191
2|2||6390|0|121|280|159|200.5|33.0895250293302|1094.91666666667|16.5035037552769|1281195|177|200.5|224|245
"""

        self.assertModule("v.rast.stats", map="zone_map", raster="map_a",
                          flags="c", column_prefix="a")
        v_db_select = SimpleModule("v.db.select", map="zone_map")

        self.runModule(v_db_select)
        self.assertLooksLike(univar_string, str(v_db_select.outputs.stdout))


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
