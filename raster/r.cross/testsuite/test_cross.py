import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRCross(TestCase):
    """Test case for r.cross module"""

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and create test raster maps"""
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)

        # Create map1: categories 1, 2, 3 as rows
        cls.runModule(
            "r.mapcalc",
            expression="map1 = if(row() == 1, 1, if(row() == 2, 2, if(row() == 3, 3, null())))",
            overwrite=True,
        )

        # Create map2: categories 1, 2, NULL an columns
        cls.runModule(
            "r.mapcalc",
            expression="map2 = if(col() == 1, 1, if(col() == 2, 2, null()))",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary maps and region"""
        cls.runModule(
            "g.remove", flags="f", type="raster", name=["map1", "map2", "crossed_map"]
        )
        cls.del_temp_region()

    def test_cross_basic(self):
        """Test basic cross product functionality"""

        module = SimpleModule(
            "r.cross", input="map1,map2", output="crossed_map", overwrite=True
        )
        self.assertModule(module)

        self.assertRasterExists(
            "crossed_map", msg="Output raster map 'crossed_map' should exist"
        )

        # Validate category mappings using r.category
        category_module = gs.parse_command(
            "r.describe", map="crossed_map", format="json"
        )

        # Define expected categories
        expected_categories = {"has_nulls": False, "ranges": [{"min": 0, "max": 8}]}

        # Compare actual and expected categories
        self.assertEqual(category_module, expected_categories)

    def test_cross_with_z_flag(self):
        """Test cross product with the -z flag to exclude NULL values"""

        module = SimpleModule(
            "r.cross",
            input="map1,map2",
            output="crossed_map",
            flags="z",
            overwrite=True,
        )
        self.assertModule(module)

        self.assertRasterExists(
            "crossed_map", msg="Output raster map 'crossed_map' should exist"
        )

        category_module = gs.parse_command(
            "r.describe", map="crossed_map", format="json"
        )

        expected_categories = {"has_nulls": True, "ranges": [{"min": 0, "max": 5}]}

        self.assertEqual(category_module, expected_categories)


if __name__ == "__main__":
    test()
