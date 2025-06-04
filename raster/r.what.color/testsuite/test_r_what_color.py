from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestRWhatColor(TestCase):
    input = "elevation"
    value = "50\n100\n116.029\n135\n156\nbogus"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.input, flags="p")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_r_what_color_plain(self):
        """Test r.what.color command for plain output format."""
        module = SimpleModule(
            "r.what.color", input=self.input, flags="i", stdin=self.value
        )
        self.assertModule(module)
        result = module.outputs.stdout.splitlines()
        expected = [
            "50: *",
            "100: 255:229:0",
            "116.029: 255:128:0",
            "135: 195:127:59",
            "156: 23:22:21",
            "*: *",
        ]

        self.assertListEqual(result, expected, "Mismatch in printed output (plain)")

    def test_r_what_color_plain_with_format_option(self):
        """Test the r.what.color command with the format option for plain text output."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="#%02X%02X%02X",
        )
        self.assertModule(module)
        result = module.outputs.stdout.splitlines()
        expected = [
            "50: *",
            "100: #FFE500",
            "116.029: #FF8000",
            "135: #C37F3B",
            "156: #171615",
            "*: *",
        ]

        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (plain) with the format option",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
