from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command


class TestVWhatRast(TestCase):
    """Unit tests for the v.what.rast module"""

    test_raster = "test_raster"
    float_raster = "float_raster"
    test_vector = "test_vector"

    @classmethod
    def setUpClass(cls):
        """Set up test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=5, w=0, e=5, res=1)
        cls.runModule(
            "r.mapcalc",
            expression=(f"{cls.test_raster} = if(col() < 5, col(), null())"),
        )
        cls.runModule(
            "r.mapcalc",
            expression=(f"{cls.float_raster} = if(col() < 5, col() / 2., 4.5)"),
        )
        cls.runModule("v.mkgrid", map=f"{cls.test_vector}", grid=[5, 5], type="point")

    @classmethod
    def tearDownClass(cls):
        """Clean up test environment"""
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[cls.test_raster, cls.float_raster],
        )
        cls.runModule(
            "g.remove",
            flags="f",
            type="vector",
            name=[cls.test_vector],
        )

    def test_plain_output_int(self):
        """Verify plain text output with integer map."""
        result = read_command(
            "v.what.rast", map=self.test_vector, raster=self.test_raster, flags="p"
        ).splitlines()
        expected = [
            "21|1",
            "22|2",
            "23|3",
            "24|4",
            "25|*",
            "16|1",
            "17|2",
            "18|3",
            "19|4",
            "20|*",
            "11|1",
            "12|2",
            "13|3",
            "14|4",
            "15|*",
            "6|1",
            "7|2",
            "8|3",
            "9|4",
            "10|*",
            "1|1",
            "2|2",
            "3|3",
            "4|4",
            "5|*",
        ]
        self.assertEqual(result, expected)

    def test_plain_output_float(self):
        """Verify plain text output with float map."""
        result = read_command(
            "v.what.rast", map=self.test_vector, raster=self.float_raster, flags="p"
        ).splitlines()
        expected = [
            "21|0.5",
            "22|1",
            "23|1.5",
            "24|2",
            "25|4.5",
            "16|0.5",
            "17|1",
            "18|1.5",
            "19|2",
            "20|4.5",
            "11|0.5",
            "12|1",
            "13|1.5",
            "14|2",
            "15|4.5",
            "6|0.5",
            "7|1",
            "8|1.5",
            "9|2",
            "10|4.5",
            "1|0.5",
            "2|1",
            "3|1.5",
            "4|2",
            "5|4.5",
        ]
        self.assertEqual(result, expected)


if __name__ == "__main__":
    test()
