from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRGrow(TestCase):

    @classmethod
    def setUpClass(cls):
        """Set up a small region and test map."""
        cls.output = "test_grow"
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)

        # Create a test map with a centered 3x3 block of 1s
        cls.runModule(
            "r.mapcalc",
            expression="test_map = if(row() > 3 && row() < 7 && col() > 3 && col() < 7, 1, null())",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up test maps after all tests."""
        cls.runModule("g.remove", type="raster", name="test_map,test_grow", flags="f")

    def tearDown(self):
        """Remove output map after each test to prevent conflicts."""
        self.runModule("g.remove", type="raster", name=self.output, flags="f")

    def test_default_growth(self):
        """Test default growth using Euclidean metric."""
        module = SimpleModule(
            "r.grow", input="test_map", output=self.output, overwrite=True
        )
        self.assertModule(module)

        self.assertRasterFitsUnivar(raster=self.output, reference="n=21")

    def test_grow_with_manhattan_metric(self):
        """Test growth with Manhattan metric and float radius of 2"""
        module = SimpleModule(
            "r.grow",
            input="test_map",
            output=self.output,
            radius=2,
            metric="manhattan",
            overwrite=True,
        )
        self.assertModule(module)

        self.assertRasterFitsUnivar(raster=self.output, reference="n=21")

    def test_grow_with_maximum_metric(self):
        """Test growth with Maximum metric."""
        module = SimpleModule(
            "r.grow",
            input="test_map",
            output=self.output,
            metric="maximum",
            overwrite=True,
        )
        self.assertModule(module)

        self.assertRasterFitsUnivar(raster=self.output, reference="n=25")

    def test_shrink_with_negative_radius(self):
        """Test shrinking with a negative radius of -2 to reduce area size."""
        module = SimpleModule(
            "r.grow", input="test_map", radius="-2", output=self.output, overwrite=True
        )
        self.assertModule(module)

        self.assertRasterFitsUnivar(raster=self.output, reference="n=1")

    def test_old_value_replacement(self):
        """Test replacing the original cells with -1 and new ones with 2."""
        module = SimpleModule(
            "r.grow",
            input="test_map",
            output=self.output,
            old=-1,
            new="2",
            overwrite=True,
        )
        self.assertModule(module)

        expected_values = {"n": 21, "sum": 15, "min": -1, "max": 2}
        self.assertRasterFitsUnivar(raster=self.output, reference=expected_values)


if __name__ == "__main__":
    test()
