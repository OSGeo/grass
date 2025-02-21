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
        category_module = SimpleModule("r.category", map="crossed_map")
        self.assertModule(category_module)

        # Parse r.category output
        category_output = category_module.outputs.stdout.strip().split("\n")
        actual_categories = {
            int(line.split("\t")[0]): line.split("\t")[1].strip()
            for line in category_output
            if "\t" in line
        }

        # Define expected categories
        expected_categories = {
            0: "category 1; NULL",
            1: "category 1; category 1",
            2: "category 1; category 2",
            3: "category 2; NULL",
            4: "category 2; category 1",
            5: "category 2; category 2",
            6: "category 3; NULL",
            7: "category 3; category 1",
            8: "category 3; category 2",
        }

        # Compare actual and expected categories
        for category, label in expected_categories.items():
            self.assertEqual(
                actual_categories[category],
                label,
                msg=f"Category {category} should have label '{label}', but got '{actual_categories[category]}'",
            )

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

        category_module = SimpleModule("r.category", map="crossed_map")
        self.assertModule(category_module)

        category_output = category_module.outputs.stdout.strip().split("\n")
        actual_categories = {
            int(line.split("\t")[0]): line.split("\t")[1].strip()
            for line in category_output
            if "\t" in line
        }

        # Define expected categories (NULL values excluded)
        expected_categories = {
            0: "category 1; category 1",
            1: "category 1; category 2",
            2: "category 2; category 1",
            3: "category 2; category 2",
            4: "category 3; category 1",
            5: "category 3; category 2",
        }

        # Compare actual and expected categories
        for category, label in expected_categories.items():
            self.assertEqual(
                actual_categories[category],
                label,
                msg=f"Category {category} should have label '{label}', but got '{actual_categories[category]}'",
            )


if __name__ == "__main__":
    test()
