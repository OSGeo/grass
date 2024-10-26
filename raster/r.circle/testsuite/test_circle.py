from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import grass.script as gs
from grass.gunittest.gmodules import SimpleModule


class TestRCircle(TestCase):

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region for testing."""
        cls.use_temp_region()
        cls.runModule("g.region", n=1000, s=0, e=1000, w=0, res=10)

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.del_temp_region()

    def tearDown(self):
        gs.run_command(
            "g.remove",
            type="raster",
            name="test_circle_binary,test_circle_distance",
            flags="f",
        )

    def test_create_circle_with_b_flag(self):
        """Test creating a binary circle with r.circle using -b flag."""
        output = "test_circle_binary"

        module = SimpleModule(
            "r.circle", output=output, coordinates=(500, 500), max=100, flags="b"
        )

        self.assertModule(module)

        self.assertRasterExists(output)

        self.assertRasterMinMax(
            map=output,
            refmin=1,
            refmax=1,
            msg="Binary circle should have category value of 1",
        )

    def test_create_circle_without_b_flag(self):
        """Test creating a circle with r.circle without -b flag."""
        output = "test_circle_distance"

        module = SimpleModule(
            "r.circle", output=output, coordinates=(500, 500), max=100
        )

        self.assertModule(module)

        self.assertRasterExists(output)

        self.assertRasterMinMax(
            map=output,
            refmin=0,
            refmax=100,
            msg="Circle should have distance values from 0 to 100",
        )


if __name__ == "__main__":
    test()
