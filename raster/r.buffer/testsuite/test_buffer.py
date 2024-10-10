from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
import grass.script as gs


class TestRBuffer(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster="roadsmajor")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(self):
        # Remove temporary maps created during tests
        gs.run_command(
            "g.remove",
            type="raster",
            name="null_map,zero_map,buf_test,buf_no_non_null,buf_ignore_zero",
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

        expected_categories = [1] + [i + 1 for i in range(len(distances) + 1)]

        self.assertRasterMinMax(
            map=output,
            refmin=min(expected_categories),
            refmax=max(expected_categories),
            msg=(
                "Buffer zones should have category values from 1 to "  # Checking if there are no abnormal values in the output raster map
                + str(max(expected_categories))
            ),
        )

        category_values = gs.read_command(
            "r.stats", flags="n", input=output
        ).splitlines()
        category_values = [int(line.split()[0]) for line in category_values]

        print("Category values:", category_values)

        unique_actual_categories = set(category_values)
        self.assertTrue(
            unique_actual_categories.issubset(set(expected_categories)),
            msg=(
                "Output categories should be a subset of expected categories: "
                + str(expected_categories)
            ),
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
        stats = gs.read_command("r.univar", map=output, flags="g")
        self.assertIn(
            "n=0", stats, msg="Output should have no non-NULL cells"
        )  # Checking an edge case where the input raster map is null

    def test_ignore_zero_values(self):
        zero_map = "zero_map"
        self.runModule("r.mapcalc", expression=f"{zero_map} = if(row() % 2 == 0, 0, 1)")

        output = "buf_ignore_zero"
        distances = [100, 200]

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

        category_values = gs.read_command(
            "r.stats", flags="n", input=output
        ).splitlines()
        category_values = [int(line.split()[0]) for line in category_values]

        print("Category values:", category_values)

        self.assertNotIn(
            0,
            category_values,
            msg="Output should not contain buffer zones around zero values",
        )  # Check if the output raster map ignored 0 values


if __name__ == "__main__":
    test()
