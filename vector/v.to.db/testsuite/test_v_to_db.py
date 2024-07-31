import json
from itertools import zip_longest

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.gunittest.gmodules import SimpleModule


class TestVToDb(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()

    def _assert_dict_almost_equal(self, d1, d2):
        self.assertEqual(set(d1.keys()), set(d2.keys()))
        for k1 in d1:
            if isinstance(k1, float):
                self.assertAlmostEqual(d1[k1], d2[k1], places=6)
            else:
                self.assertEqual(d1[k1], d2[k1])

    def test_json(self):
        reference = {
            "totals": {"length": 10426.657857419743},
            "records": [
                {"category": 1, "length": 4554.943058982206},
                {"category": 2, "length": 5871.714798437537},
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "busroute6",
            flags="p",
            option="length",
            type="line",
            format="json",
        )
        self.runModule(module)
        result = json.loads(module.outputs.stdout)

        self._assert_dict_almost_equal(reference["totals"], result["totals"])
        for record1, record2 in zip_longest(reference["records"], result["records"]):
            self._assert_dict_almost_equal(record1, record2)

    def test_json2(self):
        reference = {
            "records": [
                {
                    "category": 11,
                    "x": 638150.7920150368,
                    "y": 220024.77915312737,
                    "z": 0,
                },
                {
                    "category": 103,
                    "x": 638287.18720843294,
                    "y": 219698.23416404743,
                    "z": 0,
                },
                {
                    "category": 104,
                    "x": 638278.98502463801,
                    "y": 219611.30807667322,
                    "z": 0,
                },
                {
                    "category": 105,
                    "x": 638306.74931247137,
                    "y": 219887.96339615693,
                    "z": 0,
                },
                {
                    "category": 106,
                    "x": 638269.46915021574,
                    "y": 219523.05001002364,
                    "z": 0,
                },
                {
                    "category": 107,
                    "x": 638232.74371348787,
                    "y": 219624.32918462454,
                    "z": 0,
                },
                {
                    "category": 108,
                    "x": 638230.51562103187,
                    "y": 219627.93192782634,
                    "z": 0,
                },
                {
                    "category": 109,
                    "x": 638262.64465328958,
                    "y": 219722.83464563562,
                    "z": 0,
                },
                {
                    "category": 110,
                    "x": 638260.06604013185,
                    "y": 219726.78790954105,
                    "z": 0,
                },
                {
                    "category": 111,
                    "x": 638495.58851612895,
                    "y": 219729.4579598321,
                    "z": 0,
                },
                {
                    "category": 112,
                    "x": 638332.230669952,
                    "y": 219728.48552823684,
                    "z": 0,
                },
                {
                    "category": 113,
                    "x": 638317.71299009491,
                    "y": 219988.20012615179,
                    "z": 0,
                },
                {
                    "category": 114,
                    "x": 638245.4447548897,
                    "y": 219808.26111248325,
                    "z": 0,
                },
                {
                    "category": 117,
                    "x": 638329.73432644235,
                    "y": 220099.23289130288,
                    "z": 0,
                },
            ]
        }
        module = SimpleModule(
            "v.to.db", "P079218", flags="p", option="coor", type="point", format="json"
        )
        self.runModule(module)
        result = json.loads(module.outputs.stdout)
        for record1, record2 in zip_longest(reference["records"], result["records"]):
            self._assert_dict_almost_equal(record1, record2)


if __name__ == "__main__":
    test()
