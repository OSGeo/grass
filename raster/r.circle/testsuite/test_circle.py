from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import grass.script as gs
from grass.gunittest.gmodules import SimpleModule


class TestRCircle(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up a temporary region for testing."""
        cls.output = "test_circle"
        cls.use_temp_region()
        cls.runModule("g.region", n=30, s=0, e=30, w=0, res=10)

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.del_temp_region()

    def tearDown(self):
        gs.run_command(
            "g.remove",
            type="raster",
            name=self.output,
            flags="f",
        )

    def test_create_circle_with_b_flag(self):
        """Test creating a binary circle with r.circle using -b flag."""

        module = SimpleModule(
            "r.circle", output=self.output, coordinates=(15, 15), max=10, flags="b"
        )

        self.assertModule(module)

        self.assertRasterExists(self.output)

        self.assertRasterMinMax(
            map=self.output,
            refmin=1,
            refmax=1,
            msg="Binary circle should have category value of 1",
        )

    def test_create_circle_without_b_flag(self):
        """Test creating a circle with r.circle without -b flag."""

        module = SimpleModule(
            "r.circle", output=self.output, coordinates=(15, 15), max=10
        )

        self.assertModule(module)

        self.assertRasterExists(self.output)

        self.assertRasterMinMax(
            map=self.output,
            refmin=0,
            refmax=10,
            msg="Circle should have distance values from 0 to 10",
        )

    def test_create_circle_with_multiplier(self):
        """Test creating a circle with r.circle with a multiplier."""

        module = SimpleModule(
            "r.circle", output=self.output, coordinates=(15, 15), max=10, multiplier=2
        )

        self.assertModule(module)

        self.assertRasterExists(self.output)

        self.assertRasterMinMax(
            map=self.output,
            refmin=0,
            refmax=20,
            msg="Circle should have distance values from 0 to 20",
        )


if __name__ == "__main__":
    test()
