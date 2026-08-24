import os
import json
from itertools import zip_longest
import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.gunittest.gmodules import SimpleModule

INPUT_POLYGON = """B 10
20.13293641   59.95688345
26.94617837   60.47397663
29.74782155   62.56499443
27.45254202   68.70650340
23.75771765   68.24937206
25.42698984   65.27444593
21.51545237   63.10353609
21.40562760   61.12318104
19.41123592   60.40477513
20.13293641   59.95688345
C 1 1
25.0 64.0
1 1
"""


class TestVToDbWGS84(TestCase):
    latlon_polygon = "finnland_wgs84"

    @classmethod
    def setUpClass(cls):
        # create a WGS84 project
        cls.runModule("g.proj", epsg=4326, project="tmp_wgs84")
        # get current project and mapset
        cls.curr_env = gs.gisenv()
        # switch to WGS84 project
        cls.runModule("g.mapset", project="tmp_wgs84", mapset="PERMANENT")
        # import testdata
        cls.runModule(
            "v.in.ascii",
            input="-",
            output=cls.latlon_polygon,
            format="standard",
            stdin_=INPUT_POLYGON,
            flags="n",
        )

    @classmethod
    def tearDownClass(cls):
        """Switch back to original project and delete tmp project."""
        cls.runModule(
            "g.mapset",
            project=cls.curr_env["LOCATION_NAME"],
            mapset=cls.curr_env["MAPSET"],
        )
        gs.try_rmdir(os.path.join(cls.curr_env["GISDBASE"], "tmp_wgs84"))

    def _assert_dict_almost_equal(self, d1, d2):
        self.assertEqual(set(d1.keys()), set(d2.keys()))
        for k1 in d1:
            if isinstance(d1[k1], float):
                # places=2 corresponds here to 14 significant digits
                self.assertAlmostEqual(d1[k1], d2[k1], places=2)
            else:
                self.assertEqual(d1[k1], d2[k1])

    def _assert_json_equal(self, module, reference):
        self.runModule(module)
        result = json.loads(module.outputs.stdout)

        self.assertCountEqual(list(reference.keys()), list(result.keys()))
        self._assert_dict_almost_equal(reference["units"], result["units"])
        self._assert_dict_almost_equal(reference["totals"], result["totals"])
        for record1, record2 in zip_longest(reference["records"], result["records"]):
            self._assert_dict_almost_equal(record1, record2)

    def test_area_size(self):
        reference = {
            "units": {"area": "square meters"},
            "totals": {"area": 251199344354.4303},
            "records": [{"category": 1, "area": 251199344354.4303}],
        }
        module = SimpleModule(
            "v.to.db",
            self.latlon_polygon,
            flags="p",
            option="area",
            format="json",
        )
        self._assert_json_equal(module, reference)


if __name__ == "__main__":
    test()
