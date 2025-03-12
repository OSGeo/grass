import json

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule


class TestVCategory(TestCase):
    bridges = "test_bridges"

    @classmethod
    def setUpClass(cls):
        """Generate test data."""
        cls.runModule(
            "v.category",
            input="bridges",
            output=cls.bridges,
            option="transfer",
            layer="1,2",
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.runModule("g.remove", flags="f", type="vector", name=[cls.bridges])

    def test_report_option_json(self):
        """Test v.category with the json output format, and report option."""
        module = SimpleModule(
            "v.category",
            input=self.bridges,
            option="report",
            format="json",
        )
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected = [
            {"type": "point", "field": 1, "count": 10938, "min": 1, "max": 10938},
            {"type": "all", "field": 1, "count": 10938, "min": 1, "max": 10938},
            {"type": "point", "field": 2, "count": 10938, "min": 1, "max": 10938},
            {"type": "all", "field": 2, "count": 10938, "min": 1, "max": 10938},
        ]

        self.assertListEqual(expected, result)

    def test_print_option_json(self):
        """Test v.category with the json output format, and print option."""
        module = SimpleModule(
            "v.category",
            input=self.bridges,
            option="print",
            format="json",
            ids="1-5",
            layer="1,2",
        )
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected = {
            "ids": [
                {
                    "id": 1,
                    "layers": [
                        {"layer": 1, "categories": [1]},
                        {"layer": 2, "categories": [1]},
                    ],
                },
                {
                    "id": 2,
                    "layers": [
                        {"layer": 1, "categories": [2]},
                        {"layer": 2, "categories": [2]},
                    ],
                },
                {
                    "id": 3,
                    "layers": [
                        {"layer": 1, "categories": [3]},
                        {"layer": 2, "categories": [3]},
                    ],
                },
                {
                    "id": 4,
                    "layers": [
                        {"layer": 1, "categories": [4]},
                        {"layer": 2, "categories": [4]},
                    ],
                },
                {
                    "id": 5,
                    "layers": [
                        {"layer": 1, "categories": [5]},
                        {"layer": 2, "categories": [5]},
                    ],
                },
            ]
        }

        self.assertDictEqual(expected, result)

    def test_layers_option_json(self):
        """Test v.category with the json output format, and layers option."""
        module = SimpleModule(
            "v.category", input=self.bridges, option="layers", format="json"
        )
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected = {"layers": [1, 2]}

        self.assertDictEqual(expected, result)

    def test_report_option_plain(self):
        """Test v.category with the plain output format, and report option."""
        module = SimpleModule("v.category", input=self.bridges, option="report")
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        expected = [
            "Layer/table: 1/test_bridges",
            "type       count        min        max",
            "point      10938          1      10938",
            "line           0          0          0",
            "boundary       0          0          0",
            "centroid       0          0          0",
            "area           0          0          0",
            "face           0          0          0",
            "kernel         0          0          0",
            "all        10938          1      10938",
            "Layer: 2",
            "type       count        min        max",
            "point      10938          1      10938",
            "line           0          0          0",
            "boundary       0          0          0",
            "centroid       0          0          0",
            "area           0          0          0",
            "face           0          0          0",
            "kernel         0          0          0",
            "all        10938          1      10938",
        ]

        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

    def test_report_option_shell(self):
        """Test v.category with the shell output format, and report option."""
        expected = [
            "1 point 10938 1 10938",
            "1 all 10938 1 10938",
            "2 point 10938 1 10938",
            "2 all 10938 1 10938",
        ]

        # Test with '-g' flag.
        module = SimpleModule(
            "v.category", input=self.bridges, option="report", flags="g"
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

        # Test with 'format=shell' option
        module = SimpleModule(
            "v.category", input=self.bridges, option="report", format="shell"
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

    def test_print_option_plain_and_shell(self):
        """Test v.category with the plain and shell output format, and report option."""
        expected = ["1|1", "2|2", "3|3", "4|4", "5|5"]
        module = SimpleModule(
            "v.category",
            input=self.bridges,
            option="print",
            ids="1-5",
            layer="1,2",
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        # Test plain output format.
        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

        module = SimpleModule(
            "v.category",
            input=self.bridges,
            option="print",
            ids="1-5",
            flags="g",
            layer="1,2",
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        # Test shell output format.
        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

    def test_layers_option_plain_and_shell(self):
        """Test v.category with the plain and shell output format, and layers option."""
        expected = ["1", "2"]
        module = SimpleModule("v.category", input=self.bridges, option="layers")
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        # Test plain output format.
        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

        module = SimpleModule(
            "v.category", input=self.bridges, option="layers", flags="g"
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        # Test shell output format.
        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
