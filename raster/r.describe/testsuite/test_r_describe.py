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
            expression="map = if(row() == 5 && col() == 5, null(), if(row() % 2 == 0, col() * 0.5, -col() * 0.5))",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.runModule(
            "g.remove", flags="f", type="raster", name=["map"]
        )
        cls.del_temp_region()

    def test_plain_describe(self):
        """Test r.describe with the default output format."""
        module = SimpleModule("r.describe", map="map")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "* -5.000000 thru -4.960784 -4.529412 thru -4.490196 -4.019608 thru -3.980392 ", 
            "-3.509804 thru -3.470588 -3.039216 thru -3.000000 -2.529412 thru -2.490196 ", 
            "-2.019608 thru -1.980392 -1.549020 thru -1.509804 -1.039216 thru -1.000000 ", 
            "-0.529412 thru -0.490196 0.450980 thru 0.490196 0.960784 thru 1.000000 ",
            "1.470588 thru 1.509804 1.941176 thru 1.980392 2.450980 thru 2.490196 ", 
            "2.960784 thru 3.000000 3.431373 thru 3.470588 3.941176 thru 3.980392 ", 
            "4.450980 thru 4.490196 4.960784 thru 5.000000"
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )
            
    def test_plain_describe_with_one_flag(self):
        """Test r.describe with the plain output format and the -1 flag."""
        module = SimpleModule("r.describe", map="map",flags="1")
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
            "4.960784-5.000000"
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )
            
    def test_plain_describe_with_r_flag(self):
        """Test r.describe with the plain output format and the -r flag."""
        module = SimpleModule("r.describe", map="map",flags="r")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["* -5.000000 thru 5.000000"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )
    
    def test_plain_describe_with_i_flag(self):
        """Test r.describe with the plain output format and the -i flag."""
        module = SimpleModule("r.describe", map="map",flags="i")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = ["* -5 thru -1 1-5"]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )
            
    def test_plain_describe_with_n_flag(self):
        """Test r.describe with the plain output format and the -n flag."""
        module = SimpleModule("r.describe", map="map",flags="n")
        self.assertModule(module)

        result = module.outputs.stdout.strip().splitlines()

        expected_results = [
            "-5.000000 thru -4.960784 -4.529412 thru -4.490196 -4.019608 thru -3.980392 ", 
            "-3.509804 thru -3.470588 -3.039216 thru -3.000000 -2.529412 thru -2.490196 ", 
            "-2.019608 thru -1.980392 -1.549020 thru -1.509804 -1.039216 thru -1.000000 ", 
            "-0.529412 thru -0.490196 0.450980 thru 0.490196 0.960784 thru 1.000000 ",
            "1.470588 thru 1.509804 1.941176 thru 1.980392 2.450980 thru 2.490196 ", 
            "2.960784 thru 3.000000 3.431373 thru 3.470588 3.941176 thru 3.980392 ", 
            "4.450980 thru 4.490196 4.960784 thru 5.000000"
        ]

        for i, component in enumerate(result):
            self.assertEqual(
                component, expected_results[i], f"Mismatch at line {i + 1}"
            )
            
    def test_json_describe(self):
        """Test r.describe with the json output format."""
        module = SimpleModule("r.describe", map="map",format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = [
            {
                "value": "*"
            },
            {
                "value": "-5.000000 thru -4.960784"
            },
            {
                "value": "-4.529412 thru -4.490196"
            },
            {
                "value": "-4.019608 thru -3.980392"
            },
            {
                "value": "-3.509804 thru -3.470588"
            },
            {
                "value": "-3.039216 thru -3.000000"
            },
            {
                "value": "-2.529412 thru -2.490196"
            },
            {
                "value": "-2.019608 thru -1.980392"
            },
            {
                "value": "-1.549020 thru -1.509804"
            },
            {
                "value": "-1.039216 thru -1.000000"
            },
            {
                "value": "-0.529412 thru -0.490196"
            },
            {
                "value": "0.450980 thru 0.490196"
            },
            {
                "value": "0.960784 thru 1.000000"
            },
            {
                "value": "1.470588 thru 1.509804"
            },
            {
                "value": "1.941176 thru 1.980392"
            },
            {
                "value": "2.450980 thru 2.490196"
            },
            {
                "value": "2.960784 thru 3.000000"
            },
            {
                "value": "3.431373 thru 3.470588"
            },
            {
                "value": "3.941176 thru 3.980392"
            },
            {
                "value": "4.450980 thru 4.490196"
            },
            {
                "value": "4.960784 thru 5.000000"
            }
        ]

        self.assertListEqual(expected_results, result)
            
    def test_json_describe_with_one_flag(self):
        """Test r.describe with the json output format and the -1 flag."""
        module = SimpleModule("r.describe", map="map",flags="1",format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = [
            {
                "value": "*"
            },
            {
                "value": "-5.000000--4.960784"
            },
            {
                "value": "-4.529412--4.490196"
            },
            {
                "value": "-4.019608--3.980392"
            },
            {
                "value": "-3.509804--3.470588"
            },
            {
                "value": "-3.039216--3.000000"
            },
            {
                "value": "-2.529412--2.490196"
            },
            {
                "value": "-2.019608--1.980392"
            },
            {
                "value": "-1.549020--1.509804"
            },
            {
                "value": "-1.039216--1.000000"
            },
            {
                "value": "-0.529412--0.490196"
            },
            {
                "value": "0.450980-0.490196"
            },
            {
                "value": "0.960784-1.000000"
            },
            {
                "value": "1.470588-1.509804"
            },
            {
                "value": "1.941176-1.980392"
            },
            {
                "value": "2.450980-2.490196"
            },
            {
                "value": "2.960784-3.000000"
            },
            {
                "value": "3.431373-3.470588"
            },
            {
                "value": "3.941176-3.980392"
            },
            {
                "value": "4.450980-4.490196"
            },
            {
                "value": "4.960784-5.000000"
            }
        ]

        self.assertListEqual(expected_results, result)
            
    def test_json_describe_with_r_flag(self):
        """Test r.describe with the json output format and the -r flag."""
        module = SimpleModule("r.describe", map="map",flags="r",format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = [
            {
                "value": "*"
            },
            {
                "value": "-5.000000 thru 5.000000"
            }
        ]

        self.assertListEqual(expected_results, result)
    
    def test_json_describe_with_i_flag(self):
        """Test r.describe with the json output format and the -i flag."""
        module = SimpleModule("r.describe", map="map",flags="i",format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = [
            {
                "value": "*"
            },
            {
                "value": "-5 thru -1"
            },
            {
                "value": "1-5"
            }
        ]

        self.assertListEqual(expected_results, result)
            
    def test_json_describe_with_n_flag(self):
        """Test r.describe with the json output format and the -n flag."""
        module = SimpleModule("r.describe", map="map",flags="n",format="json")
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected_results = [
            {
                "value": "-5.000000 thru -4.960784"
            },
            {
                "value": "-4.529412 thru -4.490196"
            },
            {
                "value": "-4.019608 thru -3.980392"
            },
            {
                "value": "-3.509804 thru -3.470588"
            },
            {
                "value": "-3.039216 thru -3.000000"
            },
            {
                "value": "-2.529412 thru -2.490196"
            },
            {
                "value": "-2.019608 thru -1.980392"
            },
            {
                "value": "-1.549020 thru -1.509804"
            },
            {
                "value": "-1.039216 thru -1.000000"
            },
            {
                "value": "-0.529412 thru -0.490196"
            },
            {
                "value": "0.450980 thru 0.490196"
            },
            {
                "value": "0.960784 thru 1.000000"
            },
            {
                "value": "1.470588 thru 1.509804"
            },
            {
                "value": "1.941176 thru 1.980392"
            },
            {
                "value": "2.450980 thru 2.490196"
            },
            {
                "value": "2.960784 thru 3.000000"
            },
            {
                "value": "3.431373 thru 3.470588"
            },
            {
                "value": "3.941176 thru 3.980392"
            },
            {
                "value": "4.450980 thru 4.490196"
            },
            {
                "value": "4.960784 thru 5.000000"
            }
        ]

        self.assertListEqual(expected_results, result)

if __name__ == "__main__":
    test()
