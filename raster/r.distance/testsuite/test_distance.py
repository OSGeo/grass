import json
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRDistance(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and generate test data."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)
        # Create 'map1' with block 1 at top-left and block 2 at bottom-right
        cls.runModule(
            "r.mapcalc",
            expression="map1 = if(row() <= 2 && col() <= 2, 1, if(row() >= 8 && col() >= 8, 2, null()))",
            overwrite=True,
        )
        # Label categories in 'map1'
        cls.runModule(
            "r.category",
            map="map1",
            rules="-",
            separator=":",
            stdin="1:top left block\n2:bottom right block\n",
        )
        # Create 'map2' with a block in the center
        cls.runModule(
            "r.mapcalc",
            expression="map2 = if(row() >= 4 && row() <=6 && col() >= 4 && col() <= 6, 1, null())",
            overwrite=True,
        )
        # Label categories in 'map2'
        cls.runModule(
            "r.category",
            map="map2",
            rules="-",
            separator=":",
            stdin="1:center block\n",
        )
        # Create 'map3' with null values
        cls.runModule("r.mapcalc", expression="map3 = null()", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.runModule(
            "g.remove", flags="f", type="raster", name=["map1", "map2", "map3"]
        )
        cls.del_temp_region()

    def test_distance(self):
        """Test distance calculation between map1 and map2."""
        module = SimpleModule("r.distance", map=("map1", "map2"))
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "1:1:2.8284271247:1.5:8.5:3.5:6.5",
            "2:1:2.8284271247:7.5:2.5:5.5:4.5",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

        # Test with explicit 'plain' format
        module = SimpleModule("r.distance", map=("map1", "map2"), format="plain")
        self.assertModule(module)

        result_plain = module.outputs.stdout.strip().splitlines()
        self.assertEqual(result_plain, result)

    def test_overlap_distance(self):
        """Test r.distance when comparing a map to itself with overlapping features."""
        module = SimpleModule("r.distance", map=("map1", "map1"), flags="o")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "1:1:0:0.5:9.5:0.5:9.5",
            "1:2:0:0.5:9.5:0.5:9.5",
            "2:1:0:0.5:9.5:0.5:9.5",
            "2:2:0:0.5:9.5:0.5:9.5",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

        # Test with explicit 'plain' format
        module = SimpleModule(
            "r.distance", map=("map1", "map1"), flags="o", format="plain"
        )
        self.assertModule(module)

        result_plain = module.outputs.stdout.strip().splitlines()
        self.assertEqual(result_plain, result)

    def test_null_distance(self):
        """Test r.distance when reporting null values with -n flag."""
        module = SimpleModule("r.distance", map=("map3", "map2"), flags="n")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["*:*:0:0.5:9.5:0.5:9.5", "*:1:2:3.5:8.5:3.5:6.5"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

        # Test with explicit 'plain' format
        module = SimpleModule(
            "r.distance", map=("map3", "map2"), flags="n", format="plain"
        )
        self.assertModule(module)

        result_plain = module.outputs.stdout.strip().splitlines()
        self.assertEqual(result_plain, result)

    def test_cat_labels(self):
        """Test r.distance when reporting category labels."""
        module = SimpleModule("r.distance", map=("map1", "map2"), flags="l")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "1:1:2.8284271247:1.5:8.5:3.5:6.5:top left block:center block",
            "2:1:2.8284271247:7.5:2.5:5.5:4.5:bottom right block:center block",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

        # Test with explicit 'plain' format
        module = SimpleModule(
            "r.distance", map=("map1", "map2"), flags="l", format="plain"
        )
        self.assertModule(module)

        result_plain = module.outputs.stdout.strip().splitlines()
        self.assertEqual(result_plain, result)

    def test_distance_csv(self):
        """Test distance calculation between map1 and map2 with CSV format."""
        module = SimpleModule("r.distance", map=("map1", "map2"), format="csv")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "from_category,to_category,distance,from_easting,from_northing,to_easting,to_northing",
            "1,1,2.8284271247,1.5,8.5,3.5,6.5",
            "2,1,2.8284271247,7.5,2.5,5.5,4.5",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_overlap_distance_csv(self):
        """Test r.distance when comparing a map to itself with overlapping features with CSV format."""
        module = SimpleModule(
            "r.distance", map=("map1", "map1"), flags="o", format="csv"
        )
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "from_category,to_category,distance,from_easting,from_northing,to_easting,to_northing",
            "1,1,0,0.5,9.5,0.5,9.5",
            "1,2,0,0.5,9.5,0.5,9.5",
            "2,1,0,0.5,9.5,0.5,9.5",
            "2,2,0,0.5,9.5,0.5,9.5",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_null_distance_csv(self):
        """Test r.distance when reporting null values with -n flag with CSV format."""
        module = SimpleModule(
            "r.distance", map=("map3", "map2"), flags="n", format="csv"
        )
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "from_category,to_category,distance,from_easting,from_northing,to_easting,to_northing",
            "*,*,0,0.5,9.5,0.5,9.5",
            "*,1,2,3.5,8.5,3.5,6.5",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_cat_labels_csv(self):
        """Test r.distance when reporting category labels with CSV format."""
        module = SimpleModule(
            "r.distance", map=("map1", "map2"), flags="l", format="csv"
        )
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "from_category,to_category,distance,from_easting,from_northing,to_easting,to_northing,from_label,to_label",
            "1,1,2.8284271247,1.5,8.5,3.5,6.5,top left block,center block",
            "2,1,2.8284271247,7.5,2.5,5.5,4.5,bottom right block,center block",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def assert_json_equal(self, expected, actual):
        if isinstance(expected, dict):
            self.assertIsInstance(actual, dict)
            self.assertCountEqual(expected.keys(), actual.keys())
            for key in expected:
                self.assert_json_equal(expected[key], actual[key])
        elif isinstance(expected, list):
            self.assertIsInstance(actual, list)
            self.assertEqual(len(expected), len(actual))
            for exp_item, act_item in zip(expected, actual, strict=True):
                self.assert_json_equal(exp_item, act_item)
        elif isinstance(expected, float):
            self.assertAlmostEqual(expected, actual, places=6)
        else:
            self.assertEqual(expected, actual)

    def test_distance_json(self):
        """Test distance calculation between map1 and map2 with JSON format."""
        module = SimpleModule("r.distance", map=("map1", "map2"), format="json")
        self.assertModule(module)

        expected_results = [
            {
                "from_cell": {"category": 1, "easting": 1.5, "northing": 8.5},
                "to_cell": {"category": 1, "easting": 3.5, "northing": 6.5},
                "distance": 2.8284271247461903,
            },
            {
                "from_cell": {"category": 2, "easting": 7.5, "northing": 2.5},
                "to_cell": {"category": 1, "easting": 5.5, "northing": 4.5},
                "distance": 2.8284271247461903,
            },
        ]

        result = json.loads(module.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_overlap_distance_json(self):
        """Test r.distance when comparing a map to itself with overlapping features with JSON format."""
        module = SimpleModule(
            "r.distance", map=("map1", "map1"), flags="o", format="json"
        )
        self.assertModule(module)

        expected_results = [
            {
                "from_cell": {"category": 1, "easting": 0.5, "northing": 9.5},
                "to_cell": {"category": 1, "easting": 0.5, "northing": 9.5},
                "distance": 0,
            },
            {
                "from_cell": {"category": 1, "easting": 0.5, "northing": 9.5},
                "to_cell": {"category": 2, "easting": 0.5, "northing": 9.5},
                "distance": 0,
            },
            {
                "from_cell": {"category": 2, "easting": 0.5, "northing": 9.5},
                "to_cell": {"category": 1, "easting": 0.5, "northing": 9.5},
                "distance": 0,
            },
            {
                "from_cell": {"category": 2, "easting": 0.5, "northing": 9.5},
                "to_cell": {"category": 2, "easting": 0.5, "northing": 9.5},
                "distance": 0,
            },
        ]

        result = json.loads(module.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_null_distance_json(self):
        """Test r.distance when reporting null values with -n flag with JSON format."""
        module = SimpleModule(
            "r.distance", map=("map3", "map2"), flags="n", format="json"
        )
        self.assertModule(module)

        expected_results = [
            {
                "from_cell": {"category": None, "easting": 0.5, "northing": 9.5},
                "to_cell": {"category": None, "easting": 0.5, "northing": 9.5},
                "distance": 0,
            },
            {
                "from_cell": {"category": None, "easting": 3.5, "northing": 8.5},
                "to_cell": {"category": 1, "easting": 3.5, "northing": 6.5},
                "distance": 2,
            },
        ]

        result = json.loads(module.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_cat_labels_json(self):
        """Test r.distance when reporting category labels with JSON format."""
        module = SimpleModule(
            "r.distance", map=("map1", "map2"), flags="l", format="json"
        )
        self.assertModule(module)

        expected_results = [
            {
                "from_cell": {
                    "category": 1,
                    "easting": 1.5,
                    "northing": 8.5,
                    "label": "top left block",
                },
                "to_cell": {
                    "category": 1,
                    "easting": 3.5,
                    "northing": 6.5,
                    "label": "center block",
                },
                "distance": 2.8284271247461903,
            },
            {
                "from_cell": {
                    "category": 2,
                    "easting": 7.5,
                    "northing": 2.5,
                    "label": "bottom right block",
                },
                "to_cell": {
                    "category": 1,
                    "easting": 5.5,
                    "northing": 4.5,
                    "label": "center block",
                },
                "distance": 2.8284271247461903,
            },
        ]

        result = json.loads(module.outputs.stdout)
        self.assert_json_equal(expected_results, result)


if __name__ == "__main__":
    test()
