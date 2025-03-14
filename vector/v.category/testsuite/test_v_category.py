import json

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

file = [
    "P  1 2",
    "2   11",
    "1   1",
    "2  2",
    "P  1 1",
    "10  12",
    "1   2",
    "P  1 3",
    "7   12",
    "2   3",
    "1   1",
    "1   3",
    "P  1 4",
    "5   12",
    "2   3",
    "2   4",
    "1   1",
    "1   3",
]


class TestVCategory(TestCase):
    test_vector = "test_vector"
    ascii_points_file = "\n".join(file)

    @classmethod
    def setUpClass(cls):
        """Generate test data."""
        cls.runModule(
            "v.in.ascii",
            flags="n",
            overwrite=True,
            input="-",
            output=cls.test_vector,
            format="standard",
            stdin=cls.ascii_points_file,
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up after tests."""
        cls.runModule("g.remove", flags="f", type="vector", name=[cls.test_vector])

    def test_report_option_json(self):
        """Test v.category with the json output format, and report option."""
        module = SimpleModule(
            "v.category",
            input=self.test_vector,
            option="report",
            format="json",
        )
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected = [
            {"type": "point", "layer": 1, "count": 6, "min": 1, "max": 3},
            {"type": "all", "layer": 1, "count": 6, "min": 1, "max": 3},
            {"type": "point", "layer": 2, "count": 4, "min": 2, "max": 4},
            {"type": "all", "layer": 2, "count": 4, "min": 2, "max": 4},
        ]

        self.assertListEqual(expected, result)

    def test_print_option_json(self):
        """Test v.category with the json output format, and print option."""
        module = SimpleModule(
            "v.category",
            input=self.test_vector,
            option="print",
            format="json",
            layer="1,2",
        )
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected = [
            {"id": 1, "layer": 1, "category": 1},
            {"id": 1, "layer": 2, "category": 2},
            {"id": 2, "layer": 1, "category": 2},
            {"id": 3, "layer": 1, "category": 1},
            {"id": 3, "layer": 1, "category": 3},
            {"id": 3, "layer": 2, "category": 3},
            {"id": 4, "layer": 1, "category": 1},
            {"id": 4, "layer": 1, "category": 3},
            {"id": 4, "layer": 2, "category": 3},
            {"id": 4, "layer": 2, "category": 4},
        ]

        self.assertListEqual(expected, result)

    def test_layers_option_json(self):
        """Test v.category with the json output format, and layers option."""
        module = SimpleModule(
            "v.category", input=self.test_vector, option="layers", format="json"
        )
        self.assertModule(module)

        result = json.loads(module.outputs.stdout)

        expected = {"layers": [1, 2]}

        self.assertDictEqual(expected, result)

    def test_report_option_plain(self):
        """Test v.category with the plain output format, and report option."""
        module = SimpleModule("v.category", input=self.test_vector, option="report")
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        expected = [
            "Layer: 1",
            "type       count        min        max",
            "point          6          1          3",
            "line           0          0          0",
            "boundary       0          0          0",
            "centroid       0          0          0",
            "area           0          0          0",
            "face           0          0          0",
            "kernel         0          0          0",
            "all            6          1          3",
            "Layer: 2",
            "type       count        min        max",
            "point          4          2          4",
            "line           0          0          0",
            "boundary       0          0          0",
            "centroid       0          0          0",
            "area           0          0          0",
            "face           0          0          0",
            "kernel         0          0          0",
            "all            4          2          4",
        ]

        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

    def test_report_option_shell(self):
        """Test v.category with the shell output format, and report option."""
        expected = [
            "1 point 6 1 3",
            "1 all 6 1 3",
            "2 point 4 2 4",
            "2 all 4 2 4",
        ]

        # Test with '-g' flag.
        module = SimpleModule(
            "v.category", input=self.test_vector, option="report", flags="g"
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

        # Test with 'format=shell' option
        module = SimpleModule(
            "v.category", input=self.test_vector, option="report", format="shell"
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

    def test_print_option_plain_and_shell(self):
        """Test v.category with the plain and shell output format, and report option."""
        expected = ["1|2", "2|", "1/3|3", "1/3|3/4"]
        module = SimpleModule(
            "v.category",
            input=self.test_vector,
            option="print",
            layer="1,2",
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        # Test plain output format.
        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

        module = SimpleModule(
            "v.category",
            input=self.test_vector,
            option="print",
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
        module = SimpleModule("v.category", input=self.test_vector, option="layers")
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        # Test plain output format.
        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")

        module = SimpleModule(
            "v.category", input=self.test_vector, option="layers", flags="g"
        )
        self.assertModule(module)
        result = module.outputs.stdout.strip().splitlines()

        # Test shell output format.
        for i, component in enumerate(result):
            self.assertEqual(component, expected[i], f"Mismatch at line {i + 1}")


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
