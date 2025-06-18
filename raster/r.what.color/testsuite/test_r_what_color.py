from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

import json


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

    def test_r_what_color_default(self):
        """Test r.what.color command for default behavior."""
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

        self.assertListEqual(result, expected, "Mismatch in printed output")

    def test_r_what_color_with_format_option(self):
        """Test the r.what.color command with the format option (printf-style)."""
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
            "Mismatch in printed output (printf-style) with the format option",
        )

    def test_r_what_color_plain_with_triplet_option(self):
        """Test r.what.color command with triplet option for plain output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="plain",
            color_format="triplet",
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

        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (plain) with the triplet option",
        )

    def test_r_what_color_plain_with_rgb_option(self):
        """Test r.what.color command with rgb option for plain output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="plain",
            color_format="rgb",
        )
        self.assertModule(module)
        result = module.outputs.stdout.splitlines()
        expected = [
            "50: *",
            "100: rgb(255, 229, 0)",
            "116.029: rgb(255, 128, 0)",
            "135: rgb(195, 127, 59)",
            "156: rgb(23, 22, 21)",
            "*: *",
        ]

        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (plain) with the rgb option",
        )

    def test_r_what_color_plain_with_hex_option(self):
        """Test r.what.color command with hex option for plain output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="plain",
            color_format="hex",
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
            "Mismatch in printed output (plain) with the hex option",
        )

        # Test r.what.color command with default color_format option for plain output format
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="plain",
        )
        self.assertModule(module)
        result = module.outputs.stdout.splitlines()
        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (plain) with the default option",
        )

    def test_r_what_color_plain_with_hsv_option(self):
        """Test r.what.color command with hsv option for plain output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="plain",
            color_format="hsv",
        )
        self.assertModule(module)
        result = module.outputs.stdout.splitlines()
        expected = [
            "50: *",
            "100: hsv(53, 100, 100)",
            "116.029: hsv(30, 100, 100)",
            "135: hsv(30, 69, 76)",
            "156: hsv(30, 8, 9)",
            "*: *",
        ]

        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (plain) with the hsv option",
        )

    def test_r_what_color_json_with_triplet_option(self):
        """Test r.what.color command with triplet option for json output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="json",
            color_format="triplet",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {"color": None, "value": 50},
            {"color": "255:229:0", "value": 100},
            {"color": "255:128:0", "value": 116.029},
            {"color": "195:127:59", "value": 135},
            {"color": "23:22:21", "value": 156},
            {"color": None, "value": None},
        ]

        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (JSON) with the triplet option",
        )

    def test_r_what_color_json_with_rgb_option(self):
        """Test r.what.color command with rgb option for json output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="json",
            color_format="rgb",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {"color": None, "value": 50},
            {"color": "rgb(255, 229, 0)", "value": 100},
            {"color": "rgb(255, 128, 0)", "value": 116.029},
            {"color": "rgb(195, 127, 59)", "value": 135},
            {"color": "rgb(23, 22, 21)", "value": 156},
            {"color": None, "value": None},
        ]

        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (JSON) with the rgb option",
        )

    def test_r_what_color_json_with_hex_option(self):
        """Test r.what.color command with hex option for json output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="json",
            color_format="hex",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {"color": None, "value": 50},
            {"color": "#FFE500", "value": 100},
            {"color": "#FF8000", "value": 116.029},
            {"color": "#C37F3B", "value": 135},
            {"color": "#171615", "value": 156},
            {"color": None, "value": None},
        ]
        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (JSON) with the hex option",
        )

        # Test r.what.color command with default color_format option for json output format
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="json",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (JSON) with the default option",
        )

    def test_r_what_color_json_with_hsv_option(self):
        """Test r.what.color command with hsv option for json output format."""
        module = SimpleModule(
            "r.what.color",
            input=self.input,
            flags="i",
            stdin=self.value,
            format="json",
            color_format="hsv",
        )
        self.assertModule(module)
        result = json.loads(module.outputs.stdout)
        expected = [
            {"color": None, "value": 50},
            {"color": "hsv(53, 100, 100)", "value": 100},
            {"color": "hsv(30, 100, 100)", "value": 116.029},
            {"color": "hsv(30, 69, 76)", "value": 135},
            {"color": "hsv(30, 8, 9)", "value": 156},
            {"color": None, "value": None},
        ]

        self.assertListEqual(
            result,
            expected,
            "Mismatch in printed output (JSON) with the hsv option",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
