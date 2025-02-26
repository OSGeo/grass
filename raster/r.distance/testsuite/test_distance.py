from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRDistance(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and generate test data."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)
        # Create 'map1' with a block at the top-left corner
        cls.runModule(
            "r.mapcalc",
            expression="map1 = if(row() <= 2 && col() <= 2, 1, null())",
            overwrite=True,
        )
        # Create 'map2' with a block in the center
        cls.runModule(
            "r.mapcalc",
            expression="map2 = if(row() >= 4 && row() <=6 && col() >= 4 && col() <= 6, 1, null())",
            overwrite=True,
        )
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

        expected_results = ["1:1:2.8284271247:1.5:8.5:3.5:6.5"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_overlap_distance(self):
        """Test r.distance when comparing a map to itself with overlapping features."""
        module = SimpleModule("r.distance", map=("map1", "map1"), flags="o")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["1:1:0:0.5:9.5:0.5:9.5"]

        self.assertEqual(
            result,
            expected_results,
            "Mismatch in r.distance output for overlapping features",
        )

    def test_null_distance(self):
        """Test r.distance when reporting null values with -n flag."""
        module = SimpleModule("r.distance", map=("map3", "map2"), flags="n")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["*:*:0:0.5:9.5:0.5:9.5", "*:1:2:3.5:8.5:3.5:6.5"]

        self.assertEqual(
            result,
            expected_results,
            "Mismatch in r.distance output for reporting null objects as *",
        )


if __name__ == "__main__":
    test()
