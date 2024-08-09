import json

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import call_module


class TestVCategory(TestCase):

    def test_d_flag(self):
        expected = [
            {"type": "point", "field": 1, "count": 10938, "min": 1, "max": 10938},
            {"type": "all", "field": 1, "count": 10938, "min": 1, "max": 10938},
        ]
        output = call_module(
            "v.category", input="bridges", option="report", format="json"
        )
        self.assertListEqual(expected, json.loads(output))


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
