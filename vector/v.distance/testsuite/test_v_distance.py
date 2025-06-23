import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVDistance(TestCase):
    def test_json(self):
        """Test json format"""
        reference = [
            {
                "from_cat": 1,
                "to_cat": 112,
                "dist": 0.1428123184481199,
                "to_attr": "8,A",
            },
            {
                "from_cat": 2,
                "to_cat": 44,
                "dist": 0.10232660032693719,
                "to_attr": "9,A",
            },
        ]

        kwargs = {
            "flags": "p",
            "from": "busroute_a",
            "to": "busstopsall",
            "upload": ["dist,to_attr"],
            "to_column": "routes",
            "format": "json",
        }
        module = SimpleModule("v.distance", **kwargs)
        self.runModule(module)
        received = json.loads(module.outputs.stdout)

        self.assertEqual(len(reference), len(received))
        for expected, actual in zip(reference, received):
            for key, expected_value in expected.items():
                self.assertIn(key, actual)
                actual_value = actual[key]
                if isinstance(expected_value, float):
                    self.assertAlmostEqual(expected_value, actual_value, places=6)
                else:
                    self.assertEqual(expected_value, actual_value)


if __name__ == "__main__":
    test()
