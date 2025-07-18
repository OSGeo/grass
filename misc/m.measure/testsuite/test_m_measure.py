from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command, parse_command


class TestMMeasure(TestCase):
    @classmethod
    def setUpClass(cls):
        """Set temporary region"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

    def test_m_measure_plain_with_area(self):
        """Test plain text output with length and area"""
        actual = read_command(
            "m.measure",
            coordinates=[
                "631969",
                "227998",
                "643325",
                "220544",
                "634067",
                "216826",
                "631969",
                "227998",
            ],
        ).splitlines()
        expected = [
            "Length:  34927.808328 meters",
            "Area:    55615370.000000 square meters",
        ]
        self.assertEqual(actual, expected)

        # Repeat with explicit plain format
        actual = read_command(
            "m.measure",
            coordinates=[
                "631969",
                "227998",
                "643325",
                "220544",
                "634067",
                "216826",
                "631969",
                "227998",
            ],
            format="plain",
        ).splitlines()
        self.assertEqual(actual, expected)

    def test_m_measure_plain_without_area(self):
        """Test plain text output with length only"""
        actual = read_command(
            "m.measure",
            coordinates=["631969", "227998", "643325", "220544", "634067", "216826"],
        ).splitlines()
        expected = ["Length:  23560.522461 meters"]
        self.assertEqual(actual, expected)

        # Repeat with explicit plain format
        actual = read_command(
            "m.measure",
            coordinates=["631969", "227998", "643325", "220544", "634067", "216826"],
            format="plain",
        ).splitlines()
        self.assertEqual(actual, expected)

    def test_m_measure_shell_with_area(self):
        """Test shell-format output with length and area"""
        actual = read_command(
            "m.measure",
            coordinates=[
                "631969",
                "227998",
                "643325",
                "220544",
                "634067",
                "216826",
                "631969",
                "227998",
            ],
            flags="g",
        ).splitlines()
        expected = [
            "units=meters,square meters",
            "length=34927.808328",
            "area=55615370.000000",
        ]
        self.assertEqual(actual, expected)

        # Repeat with explicit shell format
        actual = read_command(
            "m.measure",
            coordinates=[
                "631969",
                "227998",
                "643325",
                "220544",
                "634067",
                "216826",
                "631969",
                "227998",
            ],
            format="shell",
        ).splitlines()
        self.assertEqual(actual, expected)

    def test_m_measure_shell_without_area(self):
        """Test shell-format output with length only"""
        actual = read_command(
            "m.measure",
            coordinates=["631969", "227998", "643325", "220544", "634067", "216826"],
            flags="g",
        ).splitlines()
        expected = ["units=meters,square meters", "length=23560.522461"]
        self.assertEqual(actual, expected)

        # Repeat with explicit shell format
        actual = read_command(
            "m.measure",
            coordinates=["631969", "227998", "643325", "220544", "634067", "216826"],
            format="shell",
        ).splitlines()
        self.assertEqual(actual, expected)

    def test_m_measure_json_with_area(self):
        """Test JSON format output with length and area"""
        actual = parse_command(
            "m.measure",
            coordinates=[
                "631969",
                "227998",
                "643325",
                "220544",
                "634067",
                "216826",
                "631969",
                "227998",
            ],
            format="json",
        )
        expected = {
            "units": {"length": "meters", "area": "square meters"},
            "length": 34927.80832838996,
            "area": 55615370,
        }
        self.assertEqual(actual, expected)

    def test_m_measure_json_without_area(self):
        """Test JSON format output with length only"""
        actual = parse_command(
            "m.measure",
            coordinates=["631969", "227998", "643325", "220544", "634067", "216826"],
            format="json",
        )
        expected = {
            "units": {"length": "meters", "area": "square meters"},
            "length": 23560.522460602522,
            "area": 0,
        }
        self.assertEqual(actual, expected)


if __name__ == "__main__":
    test()
