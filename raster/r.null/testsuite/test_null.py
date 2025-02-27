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

        # Create map1: categories 1, 2.5, 3 as rows
        cls.runModule(
            "r.mapcalc",
            expression="map2 = if(row() == 1, 1, if(row() == 2, 2.5, if(row() == 3, 3, null())))",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary maps and region"""
        cls.runModule("g.remove", flags="f", type="raster", name=["map1", "map2"])
        cls.del_temp_region()

    def test_basic(self):
        """Verify module execute with -i flag"""
        module = SimpleModule("r.null", map="map1", setnull="1", flags="i")
        self.assertModule(module)

        # Validate category mappings using r.category
        category_output = gs.parse_command("r.category", map="map1")
        expected_output = {"2": None, "3": None}

        self.assertEqual(category_output, expected_output)

    def test_float_flag(self):
        """Verify module execute with -f flag"""
        module = SimpleModule("r.null", map="map2", setnull="1", flags="f")
        self.assertModule(module)

        category_output = gs.parse_command("r.describe", map="map2")
        expected_output = {"* 2.500000-2.501961 2.998039-3.000000": None}

        self.assertEqual(category_output, expected_output)

    def test_fill_nulls(self):
        """Verify module fills nulls"""
        module = SimpleModule("r.null", map="map1", null="1")
        self.assertModule(module)

        category_output = gs.parse_command("r.category", map="map1")
        expected_output = {"1": None, "2": None, "3": None}

        self.assertEqual(category_output, expected_output)


if __name__ == "__main__":
    test()
