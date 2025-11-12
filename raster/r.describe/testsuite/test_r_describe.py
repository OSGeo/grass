import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRDescribe(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and generate test data."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, res=1)

        cls.runModule(
            "r.mapcalc",
            expression="int_map = if(row() % 2 == 0, col(), -col())",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression="float_map = if(row() == 5 && col() == 5, null(), if(row() % 2 == 0, col() * 0.5, -col() * 0.5))",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.runModule(
            "g.remove", flags="f", type="raster", name=["int_map", "float_map"]
        )
        cls.del_temp_region()

    def test_plain_describe_float(self):
        """Test r.describe with the default output format and a float-type map."""
        module = SimpleModule("r.describe", map="float_map")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "* -5.000000 thru -4.960784 -4.529412 thru -4.490196 -4.019608 thru -3.980392 ",
            "-3.509804 thru -3.470588 -3.039216 thru -3.000000 -2.529412 thru -2.490196 ",
            "-2.019608 thru -1.980392 -1.549020 thru -1.509804 -1.039216 thru -1.000000 ",
            "-0.529412 thru -0.490196 0.450980 thru 0.490196 0.960784 thru 1.000000 ",
            "1.470588 thru 1.509804 1.941176 thru 1.980392 2.450980 thru 2.490196 ",
            "2.960784 thru 3.000000 3.431373 thru 3.470588 3.941176 thru 3.980392 ",
            "4.450980 thru 4.490196 4.960784 thru 5.000000",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_one_flag_float(self):
        """Test r.describe with the plain output format, the -1 flag, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", flags="1")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "*",
            "-5.000000--4.960784",
            "-4.529412--4.490196",
            "-4.019608--3.980392",
            "-3.509804--3.470588",
            "-3.039216--3.000000",
            "-2.529412--2.490196",
            "-2.019608--1.980392",
            "-1.549020--1.509804",
            "-1.039216--1.000000",
            "-0.529412--0.490196",
            "0.450980-0.490196",
            "0.960784-1.000000",
            "1.470588-1.509804",
            "1.941176-1.980392",
            "2.450980-2.490196",
            "2.960784-3.000000",
            "3.431373-3.470588",
            "3.941176-3.980392",
            "4.450980-4.490196",
            "4.960784-5.000000",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_r_flag_float(self):
        """Test r.describe with the plain output format, the -r flag, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", flags="r")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["* -5.000000 thru 5.000000"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_i_flag_float(self):
        """Test r.describe with the plain output format, the -i flag, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", flags="i")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["* -5 thru -1 1-5"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_n_flag_float(self):
        """Test r.describe with the plain output format, the -n flag, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", flags="n")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "-5.000000 thru -4.960784 -4.529412 thru -4.490196 -4.019608 thru -3.980392 ",
            "-3.509804 thru -3.470588 -3.039216 thru -3.000000 -2.529412 thru -2.490196 ",
            "-2.019608 thru -1.980392 -1.549020 thru -1.509804 -1.039216 thru -1.000000 ",
            "-0.529412 thru -0.490196 0.450980 thru 0.490196 0.960784 thru 1.000000 ",
            "1.470588 thru 1.509804 1.941176 thru 1.980392 2.450980 thru 2.490196 ",
            "2.960784 thru 3.000000 3.431373 thru 3.470588 3.941176 thru 3.980392 ",
            "4.450980 thru 4.490196 4.960784 thru 5.000000",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_json_describe_float(self):
        """Test r.describe with the json output format, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "has_nulls": True,
            "ranges": [
                {"min": -5, "max": -4.96078431372549},
                {"min": -4.529411764705882, "max": -4.490196078431373},
                {"min": -4.019607843137255, "max": -3.980392156862745},
                {"min": -3.5098039215686274, "max": -3.4705882352941178},
                {"min": -3.0392156862745097, "max": -3},
                {"min": -2.5294117647058822, "max": -2.4901960784313726},
                {"min": -2.019607843137255, "max": -1.9803921568627452},
                {"min": -1.549019607843137, "max": -1.5098039215686274},
                {"min": -1.0392156862745097, "max": -1},
                {"min": -0.5294117647058822, "max": -0.4901960784313726},
                {"min": 0.4509803921568629, "max": 0.4901960784313726},
                {"min": 0.9607843137254903, "max": 1},
                {"min": 1.4705882352941178, "max": 1.5098039215686274},
                {"min": 1.9411764705882355, "max": 1.9803921568627452},
                {"min": 2.450980392156863, "max": 2.4901960784313726},
                {"min": 2.9607843137254903, "max": 3},
                {"min": 3.431372549019608, "max": 3.4705882352941178},
                {"min": 3.9411764705882355, "max": 3.980392156862745},
                {"min": 4.450980392156863, "max": 4.490196078431373},
                {"min": 4.96078431372549, "max": 5},
            ],
        }

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_one_flag_float(self):
        """Test r.describe with the json output format, the -1 flag, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", flags="1", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "has_nulls": True,
            "ranges": [
                {"min": -5, "max": -4.96078431372549},
                {"min": -4.529411764705882, "max": -4.490196078431373},
                {"min": -4.019607843137255, "max": -3.980392156862745},
                {"min": -3.5098039215686274, "max": -3.4705882352941178},
                {"min": -3.0392156862745097, "max": -3},
                {"min": -2.5294117647058822, "max": -2.4901960784313726},
                {"min": -2.019607843137255, "max": -1.9803921568627452},
                {"min": -1.549019607843137, "max": -1.5098039215686274},
                {"min": -1.0392156862745097, "max": -1},
                {"min": -0.5294117647058822, "max": -0.4901960784313726},
                {"min": 0.4509803921568629, "max": 0.4901960784313726},
                {"min": 0.9607843137254903, "max": 1},
                {"min": 1.4705882352941178, "max": 1.5098039215686274},
                {"min": 1.9411764705882355, "max": 1.9803921568627452},
                {"min": 2.450980392156863, "max": 2.4901960784313726},
                {"min": 2.9607843137254903, "max": 3},
                {"min": 3.431372549019608, "max": 3.4705882352941178},
                {"min": 3.9411764705882355, "max": 3.980392156862745},
                {"min": 4.450980392156863, "max": 4.490196078431373},
                {"min": 4.96078431372549, "max": 5},
            ],
        }

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_r_flag_float(self):
        """Test r.describe with the json output format, the -r flag, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", flags="r", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {"has_nulls": True, "ranges": [{"min": -5, "max": 5}]}

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_i_flag_float(self):
        """Test r.describe with the json output format and the -i flag."""
        module = SimpleModule("r.describe", map="float_map", flags="i", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "has_nulls": True,
            "ranges": [{"min": -5, "max": -1}, {"min": 1, "max": 5}],
        }

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_n_flag_float(self):
        """Test r.describe with the json output format, the -n flag, and a float-type map"""
        module = SimpleModule("r.describe", map="float_map", flags="n", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "ranges": [
                {"min": -5, "max": -4.96078431372549},
                {"min": -4.529411764705882, "max": -4.490196078431373},
                {"min": -4.019607843137255, "max": -3.980392156862745},
                {"min": -3.5098039215686274, "max": -3.4705882352941178},
                {"min": -3.0392156862745097, "max": -3},
                {"min": -2.5294117647058822, "max": -2.4901960784313726},
                {"min": -2.019607843137255, "max": -1.9803921568627452},
                {"min": -1.549019607843137, "max": -1.5098039215686274},
                {"min": -1.0392156862745097, "max": -1},
                {"min": -0.5294117647058822, "max": -0.4901960784313726},
                {"min": 0.4509803921568629, "max": 0.4901960784313726},
                {"min": 0.9607843137254903, "max": 1},
                {"min": 1.4705882352941178, "max": 1.5098039215686274},
                {"min": 1.9411764705882355, "max": 1.9803921568627452},
                {"min": 2.450980392156863, "max": 2.4901960784313726},
                {"min": 2.9607843137254903, "max": 3},
                {"min": 3.431372549019608, "max": 3.4705882352941178},
                {"min": 3.9411764705882355, "max": 3.980392156862745},
                {"min": 4.450980392156863, "max": 4.490196078431373},
                {"min": 4.96078431372549, "max": 5},
            ]
        }

        self.assertDictEqual(expected_results, result)

    def test_plain_describe_int(self):
        """Test r.describe with the default output format, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "-10 thru -1 1-10",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_one_flag_int(self):
        """Test r.describe with the plain output format, the -1 flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="1")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "-10",
            "-9",
            "-8",
            "-7",
            "-6",
            "-5",
            "-4",
            "-3",
            "-2",
            "-1",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_r_flag_int(self):
        """Test r.describe with the plain output format, the -r flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="r")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["-10 thru -1", "1 thru 10"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_i_flag_int(self):
        """Test r.describe with the plain output format, the -i flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="i")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["-10 thru -1 1-10"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_plain_describe_with_n_flag_int(self):
        """Test r.describe with the plain output format, the -n flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="n")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["-10 thru -1 1-10"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )

    def test_json_describe_int(self):
        """Test r.describe with the json output format, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "has_nulls": False,
            "ranges": [{"min": -10, "max": -1}, {"min": 1, "max": 10}],
        }

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_one_flag_int(self):
        """Test r.describe with the json output format, the -1 flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="1", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "has_nulls": False,
            "values": [
                -10,
                -9,
                -8,
                -7,
                -6,
                -5,
                -4,
                -3,
                -2,
                -1,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                9,
                10,
            ],
        }

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_r_flag_int(self):
        """Test r.describe with the json output format, the -r flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="r", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "has_nulls": False,
            "ranges": [{"min": -10, "max": -1}, {"min": 1, "max": 10}],
        }

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_i_flag_int(self):
        """Test r.describe with the json output format, the -i flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="i", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {
            "has_nulls": False,
            "ranges": [{"min": -10, "max": -1}, {"min": 1, "max": 10}],
        }

        self.assertDictEqual(expected_results, result)

    def test_json_describe_with_n_flag_int(self):
        """Test r.describe with the json output format, the -n flag, and a int-type map"""
        module = SimpleModule("r.describe", map="int_map", flags="n", format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = {"ranges": [{"min": -10, "max": -1}, {"min": 1, "max": 10}]}

        self.assertDictEqual(expected_results, result)


if __name__ == "__main__":
    test()
