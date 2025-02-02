import json
from itertools import zip_longest

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
                "distances": [
                    {"value": 0.1428123184481199, "name": "dist"},
                    {"value": "8,A", "name": "to_attr"},
                ],
            },
            {
                "from_cat": 2,
                "to_cat": 44,
                "distances": [
                    {"value": 0.10232660032693719, "name": "dist"},
                    {"value": "9,A", "name": "to_attr"},
                ],
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

        for first, second in zip_longest(reference, received):
            self.assertEqual(first["from_cat"], second["from_cat"])
            self.assertEqual(first["to_cat"], second["to_cat"])
            for f_d, s_d in zip_longest(first["distances"], second["distances"]):
                self.assertEqual(f_d["name"], s_d["name"])
                self.assertAlmostEqual(f_d["value"], s_d["value"], places=6)
            self.assertEqual(reference, received)


if __name__ == "__main__":
    test()
