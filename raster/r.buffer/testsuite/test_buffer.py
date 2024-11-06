from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
import grass.script as gs


class TestRBuffer(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", n=223000, s=220000, w=640000, e=643000, nsres=100)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        # Remove temporary maps created during tests
        gs.run_command(
            "g.remove",
            type="raster",
            name="buf_test,zero_map,buf_no_non_null,null_map,buf_ignore_zero",
            flags="f",
        )

    def test_buffer_creation(self):
        output = "buf_test"
        distances = [100, 200, 300, 400, 500]

        module = SimpleModule(
            "r.buffer",
            input="roadsmajor",
            output=output,
            distances=distances,
            overwrite=True,
        )
        self.assertModule(module)

        self.assertRasterExists(output)

        expected_categories = [i + 1 for i in range(len(distances) + 1)]

        self.assertRasterMinMax(
            map=output,
            refmin=min(expected_categories),
            refmax=max(expected_categories),
            msg=f"Buffer zones should have category values from 1 to {max(expected_categories)}",
        )

    def test_no_non_null_values(self):
        null_map = "null_map"
        self.runModule("r.mapcalc", expression=f"{null_map} = null()")

        output = "buf_no_non_null"
        distances = [100, 200, 300]

        module = SimpleModule(
            "r.buffer",
            input=null_map,
            output=output,
            distances=distances,
            overwrite=True,
        )
        self.assertModule(module)

        self.assertRasterExists(output)

        expected_stats = {"n": 0}
        self.assertRasterFitsUnivar(output, reference=expected_stats)

    def test_ignore_zero_values(self):
        zero_map = "zero_map"
        self.runModule("r.mapcalc", expression=f"{zero_map} = 0")

        output = "buf_ignore_zero"
        distances = [100]

        module = SimpleModule(
            "r.buffer",
            input=zero_map,
            output=output,
            distances=distances,
            flags="z",
            overwrite=True,
        )
        self.assertModule(module)

        self.assertRasterExists(output)

        expected_stats = {"n": 0}
        self.assertRasterFitsUnivar(output, reference=expected_stats)


if __name__ == "__main__":
    test()
