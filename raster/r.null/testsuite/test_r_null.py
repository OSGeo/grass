import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRNull(TestCase):
    """Test case for r.null module"""

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and create test raster maps"""
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)

        # Create map1: categories 1, 2, 3 as rows
        cls.runModule(
            "r.mapcalc",
            expression="map_basic = row()",
            overwrite=True,
        )

        # Create map1: categories null, 2, 3 as rows
        cls.runModule(
            "r.mapcalc",
            expression="map_fill_nulls = if(row() == 1, null(), if(row() == 2, 2, if(row() == 3, 3, null())))",
            overwrite=True,
        )

        # Create map1: categories 1, 2.5, 3 as rows
        cls.runModule(
            "r.mapcalc",
            expression="map2 = if(row() == 1, 1, if(row() == 2, 2.5, if(row() == 3, 3, null())))",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary maps and region"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=["map_basic", "map_fill_nulls", "map2"],
        )
        cls.del_temp_region()

    def test_basic(self):
        """Verify module execute with -i flag"""
        module = SimpleModule("r.null", map="map_basic", setnull="1", flags="i")
        self.assertModule(module)

        # Validate category mappings using r.category
        category_output = gs.parse_command(
            "r.describe", map="map_basic", format="json", flags="1"
        )
        expected_output = {"has_nulls": True, "values": [2, 3]}

        self.assertEqual(category_output, expected_output)

    def test_float_flag(self):
        """Verify module execute with -f flag"""
        module = SimpleModule("r.null", map="map2", setnull="1", flags="f")
        self.assertModule(module)

        category_output = gs.parse_command(
            "r.describe", map="map2", format="json", flags="r"
        )
        expected_output = {"has_nulls": True, "ranges": [{"min": 2.5, "max": 3}]}

        self.assertEqual(category_output, expected_output)

    def test_fill_nulls(self):
        """Verify module fills nulls"""
        module = SimpleModule("r.null", map="map_fill_nulls", null="1")
        self.assertModule(module)

        category_output = gs.parse_command(
            "r.describe", map="map_fill_nulls", format="json", flags="1"
        )
        expected_output = {"has_nulls": False, "values": [1, 2, 3]}

        self.assertEqual(category_output, expected_output)


if __name__ == "__main__":
    test()
