"""
Name:       v.vect.stats test
Purpose:    Tests v.vect.stats and its flags/options.

Author:     Nishant Bansal
Copyright:  (C) 2025 by Nishant Bansal and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


output1 = """area_cat|count|sum
1|0|null
2|0|null
3|0|null
4|0|null
5|0|null
6|0|null
7|0|null
8|0|null
9|1|7
10|0|null
11|0|null
12|0|null
13|1|10
14|0|null
15|0|null
16|0|null
17|0|null
18|0|null
19|0|null
20|0|null
21|0|null
22|0|null
23|0|null
24|0|null
25|0|null
26|0|null
27|0|null
28|1|6
29|1|146
30|0|null
31|0|null
32|0|null
33|0|null
34|0|null
35|0|null
36|0|null
37|0|null
38|0|null
39|1|8
40|2|9
41|0|null
42|0|null
43|1|9
44|0|null
"""

output2 = """area_cat|count|average
1|0|null
2|0|null
3|0|null
4|0|null
5|0|null
6|0|null
7|0|null
8|0|null
9|1|7
10|0|null
11|0|null
12|0|null
13|1|10
14|0|null
15|0|null
16|0|null
17|0|null
18|0|null
19|0|null
20|0|null
21|0|null
22|0|null
23|0|null
24|0|null
25|0|null
26|0|null
27|0|null
28|1|6
29|1|146
30|0|null
31|0|null
32|0|null
33|0|null
34|0|null
35|0|null
36|0|null
37|0|null
38|0|null
39|1|8
40|2|4.5
41|0|null
42|0|null
43|1|9
44|0|null
"""

output3 = """area_cat|count|variance
1|0|null
2|0|null
3|0|null
4|0|null
5|0|null
6|0|null
7|0|null
8|0|null
9|1|0
10|0|null
11|0|null
12|0|null
13|1|0
14|0|null
15|0|null
16|0|null
17|0|null
18|0|null
19|0|null
20|0|null
21|0|null
22|0|null
23|0|null
24|0|null
25|0|null
26|0|null
27|0|null
28|1|0
29|1|0
30|0|null
31|0|null
32|0|null
33|0|null
34|0|null
35|0|null
36|0|null
37|0|null
38|0|null
39|1|0
40|2|0.25
41|0|null
42|0|null
43|1|0
44|0|null
"""

output4 = """area_cat|count|range
1|0|null
2|0|null
3|0|null
4|0|null
5|0|null
6|0|null
7|0|null
8|0|null
9|1|0
10|0|null
11|0|null
12|0|null
13|1|0
14|0|null
15|0|null
16|0|null
17|0|null
18|0|null
19|0|null
20|0|null
21|0|null
22|0|null
23|0|null
24|0|null
25|0|null
26|0|null
27|0|null
28|1|0
29|1|0
30|0|null
31|0|null
32|0|null
33|0|null
34|0|null
35|0|null
36|0|null
37|0|null
38|0|null
39|1|0
40|2|1
41|0|null
42|0|null
43|1|0
44|0|null
"""

output5 = """area_cat|count|max_cat
1|0|null
2|0|null
3|0|null
4|0|null
5|0|null
6|0|null
7|0|null
8|0|null
9|1|7
10|0|null
11|0|null
12|0|null
13|1|10
14|0|null
15|0|null
16|0|null
17|0|null
18|0|null
19|0|null
20|0|null
21|0|null
22|0|null
23|0|null
24|0|null
25|0|null
26|0|null
27|0|null
28|1|6
29|1|146
30|0|null
31|0|null
32|0|null
33|0|null
34|0|null
35|0|null
36|0|null
37|0|null
38|0|null
39|1|8
40|2|5
41|0|null
42|0|null
43|1|9
44|0|null
"""

output6 = """area_cat|count|mode
1|0|null
2|0|null
3|0|null
4|0|null
5|0|null
6|0|null
7|0|null
8|0|null
9|1|7
10|0|null
11|0|null
12|0|null
13|1|10
14|0|null
15|0|null
16|0|null
17|0|null
18|0|null
19|0|null
20|0|null
21|0|null
22|0|null
23|0|null
24|0|null
25|0|null
26|0|null
27|0|null
28|1|6
29|1|146
30|0|null
31|0|null
32|0|null
33|0|null
34|0|null
35|0|null
36|0|null
37|0|null
38|0|null
39|1|8
40|2|4
41|0|null
42|0|null
43|1|9
44|0|null
"""


class TestVectStats(TestCase):
    input = "hospitals"
    areas = "zipcodes_wake"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def test_sum(self):
        """Testing method sum"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="sum",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output1, actual=v_vect_stats.outputs.stdout)

        # Repeat with explicit plain format
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="sum",
            points_column="cat",
            flags="p",
            format="plain",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output1, actual=v_vect_stats.outputs.stdout)

    def test_average(self):
        """Testing method average"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="average",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output2, actual=v_vect_stats.outputs.stdout)

        # Repeat with explicit plain format
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="average",
            points_column="cat",
            flags="p",
            format="plain",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output2, actual=v_vect_stats.outputs.stdout)

    def test_variance(self):
        """Testing method variance"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="variance",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output3, actual=v_vect_stats.outputs.stdout)

        # Repeat with explicit plain format
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="variance",
            points_column="cat",
            flags="p",
            format="plain",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output3, actual=v_vect_stats.outputs.stdout)

    def test_range(self):
        """Testing method range"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="range",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output4, actual=v_vect_stats.outputs.stdout)

        # Repeat with explicit plain format
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="range",
            points_column="cat",
            flags="p",
            format="plain",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output4, actual=v_vect_stats.outputs.stdout)

    def test_maxcat(self):
        """Testing method max_cat"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="max_cat",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output5, actual=v_vect_stats.outputs.stdout)

        # Repeat with explicit plain format
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="max_cat",
            points_column="cat",
            flags="p",
            format="plain",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output5, actual=v_vect_stats.outputs.stdout)

    def test_mode(self):
        """Testing method mode"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="mode",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output6, actual=v_vect_stats.outputs.stdout)

        # Repeat with explicit plain format
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="mode",
            points_column="cat",
            flags="p",
            format="plain",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output6, actual=v_vect_stats.outputs.stdout)

    def test_sum_csv(self):
        """Testing method sum with CSV output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="sum",
            points_column="cat",
            flags="p",
            format="csv",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(
            reference=output1.replace("|", ","), actual=v_vect_stats.outputs.stdout
        )

    def test_average_csv(self):
        """Testing method average with CSV output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="average",
            points_column="cat",
            flags="p",
            format="csv",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(
            reference=output2.replace("|", ","), actual=v_vect_stats.outputs.stdout
        )

    def test_variance_csv(self):
        """Testing method variance with CSV output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="variance",
            points_column="cat",
            flags="p",
            format="csv",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(
            reference=output3.replace("|", ","), actual=v_vect_stats.outputs.stdout
        )

    def test_range_csv(self):
        """Testing method range with CSV output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="range",
            points_column="cat",
            flags="p",
            format="csv",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(
            reference=output4.replace("|", ","), actual=v_vect_stats.outputs.stdout
        )

    def test_maxcat_csv(self):
        """Testing method max_cat with CSV output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="max_cat",
            points_column="cat",
            flags="p",
            format="csv",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(
            reference=output5.replace("|", ","), actual=v_vect_stats.outputs.stdout
        )

    def test_mode_csv(self):
        """Testing method mode with CSV output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="mode",
            points_column="cat",
            flags="p",
            format="csv",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(
            reference=output6.replace("|", ","), actual=v_vect_stats.outputs.stdout
        )

    def assert_json_equal(self, expected, actual):
        if isinstance(expected, dict):
            self.assertIsInstance(actual, dict)
            self.assertCountEqual(expected.keys(), actual.keys())
            for key in expected:
                self.assert_json_equal(expected[key], actual[key])
        elif isinstance(expected, list):
            self.assertIsInstance(actual, list)
            self.assertEqual(len(expected), len(actual))
            for exp_item, act_item in zip(expected, actual, strict=True):
                self.assert_json_equal(exp_item, act_item)
        elif isinstance(expected, float):
            self.assertAlmostEqual(expected, actual, places=6)
        else:
            self.assertEqual(expected, actual)

    def test_sum_json(self):
        """Testing method sum with JSON output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="sum",
            points_column="cat",
            flags="p",
            format="json",
        )
        self.assertModule(v_vect_stats)

        expected_results = [
            {"category": 1, "count": 0, "sum": None},
            {"category": 2, "count": 0, "sum": None},
            {"category": 3, "count": 0, "sum": None},
            {"category": 4, "count": 0, "sum": None},
            {"category": 5, "count": 0, "sum": None},
            {"category": 6, "count": 0, "sum": None},
            {"category": 7, "count": 0, "sum": None},
            {"category": 8, "count": 0, "sum": None},
            {"category": 9, "count": 1, "sum": 7},
            {"category": 10, "count": 0, "sum": None},
            {"category": 11, "count": 0, "sum": None},
            {"category": 12, "count": 0, "sum": None},
            {"category": 13, "count": 1, "sum": 10},
            {"category": 14, "count": 0, "sum": None},
            {"category": 15, "count": 0, "sum": None},
            {"category": 16, "count": 0, "sum": None},
            {"category": 17, "count": 0, "sum": None},
            {"category": 18, "count": 0, "sum": None},
            {"category": 19, "count": 0, "sum": None},
            {"category": 20, "count": 0, "sum": None},
            {"category": 21, "count": 0, "sum": None},
            {"category": 22, "count": 0, "sum": None},
            {"category": 23, "count": 0, "sum": None},
            {"category": 24, "count": 0, "sum": None},
            {"category": 25, "count": 0, "sum": None},
            {"category": 26, "count": 0, "sum": None},
            {"category": 27, "count": 0, "sum": None},
            {"category": 28, "count": 1, "sum": 6},
            {"category": 29, "count": 1, "sum": 146},
            {"category": 30, "count": 0, "sum": None},
            {"category": 31, "count": 0, "sum": None},
            {"category": 32, "count": 0, "sum": None},
            {"category": 33, "count": 0, "sum": None},
            {"category": 34, "count": 0, "sum": None},
            {"category": 35, "count": 0, "sum": None},
            {"category": 36, "count": 0, "sum": None},
            {"category": 37, "count": 0, "sum": None},
            {"category": 38, "count": 0, "sum": None},
            {"category": 39, "count": 1, "sum": 8},
            {"category": 40, "count": 2, "sum": 9},
            {"category": 41, "count": 0, "sum": None},
            {"category": 42, "count": 0, "sum": None},
            {"category": 43, "count": 1, "sum": 9},
            {"category": 44, "count": 0, "sum": None},
        ]

        result = json.loads(v_vect_stats.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_average_json(self):
        """Testing method average with JSON output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="average",
            points_column="cat",
            flags="p",
            format="json",
        )
        self.assertModule(v_vect_stats)

        expected_results = [
            {"category": 1, "count": 0, "average": None},
            {"category": 2, "count": 0, "average": None},
            {"category": 3, "count": 0, "average": None},
            {"category": 4, "count": 0, "average": None},
            {"category": 5, "count": 0, "average": None},
            {"category": 6, "count": 0, "average": None},
            {"category": 7, "count": 0, "average": None},
            {"category": 8, "count": 0, "average": None},
            {"category": 9, "count": 1, "average": 7},
            {"category": 10, "count": 0, "average": None},
            {"category": 11, "count": 0, "average": None},
            {"category": 12, "count": 0, "average": None},
            {"category": 13, "count": 1, "average": 10},
            {"category": 14, "count": 0, "average": None},
            {"category": 15, "count": 0, "average": None},
            {"category": 16, "count": 0, "average": None},
            {"category": 17, "count": 0, "average": None},
            {"category": 18, "count": 0, "average": None},
            {"category": 19, "count": 0, "average": None},
            {"category": 20, "count": 0, "average": None},
            {"category": 21, "count": 0, "average": None},
            {"category": 22, "count": 0, "average": None},
            {"category": 23, "count": 0, "average": None},
            {"category": 24, "count": 0, "average": None},
            {"category": 25, "count": 0, "average": None},
            {"category": 26, "count": 0, "average": None},
            {"category": 27, "count": 0, "average": None},
            {"category": 28, "count": 1, "average": 6},
            {"category": 29, "count": 1, "average": 146},
            {"category": 30, "count": 0, "average": None},
            {"category": 31, "count": 0, "average": None},
            {"category": 32, "count": 0, "average": None},
            {"category": 33, "count": 0, "average": None},
            {"category": 34, "count": 0, "average": None},
            {"category": 35, "count": 0, "average": None},
            {"category": 36, "count": 0, "average": None},
            {"category": 37, "count": 0, "average": None},
            {"category": 38, "count": 0, "average": None},
            {"category": 39, "count": 1, "average": 8},
            {"category": 40, "count": 2, "average": 4.5},
            {"category": 41, "count": 0, "average": None},
            {"category": 42, "count": 0, "average": None},
            {"category": 43, "count": 1, "average": 9},
            {"category": 44, "count": 0, "average": None},
        ]

        result = json.loads(v_vect_stats.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_variance_json(self):
        """Testing method variance with JSON output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="variance",
            points_column="cat",
            flags="p",
            format="json",
        )
        self.assertModule(v_vect_stats)

        expected_results = [
            {"category": 1, "count": 0, "variance": None},
            {"category": 2, "count": 0, "variance": None},
            {"category": 3, "count": 0, "variance": None},
            {"category": 4, "count": 0, "variance": None},
            {"category": 5, "count": 0, "variance": None},
            {"category": 6, "count": 0, "variance": None},
            {"category": 7, "count": 0, "variance": None},
            {"category": 8, "count": 0, "variance": None},
            {"category": 9, "count": 1, "variance": 0},
            {"category": 10, "count": 0, "variance": None},
            {"category": 11, "count": 0, "variance": None},
            {"category": 12, "count": 0, "variance": None},
            {"category": 13, "count": 1, "variance": 0},
            {"category": 14, "count": 0, "variance": None},
            {"category": 15, "count": 0, "variance": None},
            {"category": 16, "count": 0, "variance": None},
            {"category": 17, "count": 0, "variance": None},
            {"category": 18, "count": 0, "variance": None},
            {"category": 19, "count": 0, "variance": None},
            {"category": 20, "count": 0, "variance": None},
            {"category": 21, "count": 0, "variance": None},
            {"category": 22, "count": 0, "variance": None},
            {"category": 23, "count": 0, "variance": None},
            {"category": 24, "count": 0, "variance": None},
            {"category": 25, "count": 0, "variance": None},
            {"category": 26, "count": 0, "variance": None},
            {"category": 27, "count": 0, "variance": None},
            {"category": 28, "count": 1, "variance": 0},
            {"category": 29, "count": 1, "variance": 0},
            {"category": 30, "count": 0, "variance": None},
            {"category": 31, "count": 0, "variance": None},
            {"category": 32, "count": 0, "variance": None},
            {"category": 33, "count": 0, "variance": None},
            {"category": 34, "count": 0, "variance": None},
            {"category": 35, "count": 0, "variance": None},
            {"category": 36, "count": 0, "variance": None},
            {"category": 37, "count": 0, "variance": None},
            {"category": 38, "count": 0, "variance": None},
            {"category": 39, "count": 1, "variance": 0},
            {"category": 40, "count": 2, "variance": 0.25},
            {"category": 41, "count": 0, "variance": None},
            {"category": 42, "count": 0, "variance": None},
            {"category": 43, "count": 1, "variance": 0},
            {"category": 44, "count": 0, "variance": None},
        ]

        result = json.loads(v_vect_stats.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_range_json(self):
        """Testing method range with JSON output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="range",
            points_column="cat",
            flags="p",
            format="json",
        )
        self.assertModule(v_vect_stats)

        expected_results = [
            {"category": 1, "count": 0, "range": None},
            {"category": 2, "count": 0, "range": None},
            {"category": 3, "count": 0, "range": None},
            {"category": 4, "count": 0, "range": None},
            {"category": 5, "count": 0, "range": None},
            {"category": 6, "count": 0, "range": None},
            {"category": 7, "count": 0, "range": None},
            {"category": 8, "count": 0, "range": None},
            {"category": 9, "count": 1, "range": 0},
            {"category": 10, "count": 0, "range": None},
            {"category": 11, "count": 0, "range": None},
            {"category": 12, "count": 0, "range": None},
            {"category": 13, "count": 1, "range": 0},
            {"category": 14, "count": 0, "range": None},
            {"category": 15, "count": 0, "range": None},
            {"category": 16, "count": 0, "range": None},
            {"category": 17, "count": 0, "range": None},
            {"category": 18, "count": 0, "range": None},
            {"category": 19, "count": 0, "range": None},
            {"category": 20, "count": 0, "range": None},
            {"category": 21, "count": 0, "range": None},
            {"category": 22, "count": 0, "range": None},
            {"category": 23, "count": 0, "range": None},
            {"category": 24, "count": 0, "range": None},
            {"category": 25, "count": 0, "range": None},
            {"category": 26, "count": 0, "range": None},
            {"category": 27, "count": 0, "range": None},
            {"category": 28, "count": 1, "range": 0},
            {"category": 29, "count": 1, "range": 0},
            {"category": 30, "count": 0, "range": None},
            {"category": 31, "count": 0, "range": None},
            {"category": 32, "count": 0, "range": None},
            {"category": 33, "count": 0, "range": None},
            {"category": 34, "count": 0, "range": None},
            {"category": 35, "count": 0, "range": None},
            {"category": 36, "count": 0, "range": None},
            {"category": 37, "count": 0, "range": None},
            {"category": 38, "count": 0, "range": None},
            {"category": 39, "count": 1, "range": 0},
            {"category": 40, "count": 2, "range": 1},
            {"category": 41, "count": 0, "range": None},
            {"category": 42, "count": 0, "range": None},
            {"category": 43, "count": 1, "range": 0},
            {"category": 44, "count": 0, "range": None},
        ]

        result = json.loads(v_vect_stats.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_maxcat_json(self):
        """Testing method max_cat with JSON output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="max_cat",
            points_column="cat",
            flags="p",
            format="json",
        )
        self.assertModule(v_vect_stats)

        expected_results = [
            {"category": 1, "count": 0, "max_cat": None},
            {"category": 2, "count": 0, "max_cat": None},
            {"category": 3, "count": 0, "max_cat": None},
            {"category": 4, "count": 0, "max_cat": None},
            {"category": 5, "count": 0, "max_cat": None},
            {"category": 6, "count": 0, "max_cat": None},
            {"category": 7, "count": 0, "max_cat": None},
            {"category": 8, "count": 0, "max_cat": None},
            {"category": 9, "count": 1, "max_cat": 7},
            {"category": 10, "count": 0, "max_cat": None},
            {"category": 11, "count": 0, "max_cat": None},
            {"category": 12, "count": 0, "max_cat": None},
            {"category": 13, "count": 1, "max_cat": 10},
            {"category": 14, "count": 0, "max_cat": None},
            {"category": 15, "count": 0, "max_cat": None},
            {"category": 16, "count": 0, "max_cat": None},
            {"category": 17, "count": 0, "max_cat": None},
            {"category": 18, "count": 0, "max_cat": None},
            {"category": 19, "count": 0, "max_cat": None},
            {"category": 20, "count": 0, "max_cat": None},
            {"category": 21, "count": 0, "max_cat": None},
            {"category": 22, "count": 0, "max_cat": None},
            {"category": 23, "count": 0, "max_cat": None},
            {"category": 24, "count": 0, "max_cat": None},
            {"category": 25, "count": 0, "max_cat": None},
            {"category": 26, "count": 0, "max_cat": None},
            {"category": 27, "count": 0, "max_cat": None},
            {"category": 28, "count": 1, "max_cat": 6},
            {"category": 29, "count": 1, "max_cat": 146},
            {"category": 30, "count": 0, "max_cat": None},
            {"category": 31, "count": 0, "max_cat": None},
            {"category": 32, "count": 0, "max_cat": None},
            {"category": 33, "count": 0, "max_cat": None},
            {"category": 34, "count": 0, "max_cat": None},
            {"category": 35, "count": 0, "max_cat": None},
            {"category": 36, "count": 0, "max_cat": None},
            {"category": 37, "count": 0, "max_cat": None},
            {"category": 38, "count": 0, "max_cat": None},
            {"category": 39, "count": 1, "max_cat": 8},
            {"category": 40, "count": 2, "max_cat": 5},
            {"category": 41, "count": 0, "max_cat": None},
            {"category": 42, "count": 0, "max_cat": None},
            {"category": 43, "count": 1, "max_cat": 9},
            {"category": 44, "count": 0, "max_cat": None},
        ]

        result = json.loads(v_vect_stats.outputs.stdout)
        self.assert_json_equal(expected_results, result)

    def test_mode_json(self):
        """Testing method mode with JSON output format"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="mode",
            points_column="cat",
            flags="p",
            format="json",
        )
        self.assertModule(v_vect_stats)

        expected_results = [
            {"category": 1, "count": 0, "mode": None},
            {"category": 2, "count": 0, "mode": None},
            {"category": 3, "count": 0, "mode": None},
            {"category": 4, "count": 0, "mode": None},
            {"category": 5, "count": 0, "mode": None},
            {"category": 6, "count": 0, "mode": None},
            {"category": 7, "count": 0, "mode": None},
            {"category": 8, "count": 0, "mode": None},
            {"category": 9, "count": 1, "mode": 7},
            {"category": 10, "count": 0, "mode": None},
            {"category": 11, "count": 0, "mode": None},
            {"category": 12, "count": 0, "mode": None},
            {"category": 13, "count": 1, "mode": 10},
            {"category": 14, "count": 0, "mode": None},
            {"category": 15, "count": 0, "mode": None},
            {"category": 16, "count": 0, "mode": None},
            {"category": 17, "count": 0, "mode": None},
            {"category": 18, "count": 0, "mode": None},
            {"category": 19, "count": 0, "mode": None},
            {"category": 20, "count": 0, "mode": None},
            {"category": 21, "count": 0, "mode": None},
            {"category": 22, "count": 0, "mode": None},
            {"category": 23, "count": 0, "mode": None},
            {"category": 24, "count": 0, "mode": None},
            {"category": 25, "count": 0, "mode": None},
            {"category": 26, "count": 0, "mode": None},
            {"category": 27, "count": 0, "mode": None},
            {"category": 28, "count": 1, "mode": 6},
            {"category": 29, "count": 1, "mode": 146},
            {"category": 30, "count": 0, "mode": None},
            {"category": 31, "count": 0, "mode": None},
            {"category": 32, "count": 0, "mode": None},
            {"category": 33, "count": 0, "mode": None},
            {"category": 34, "count": 0, "mode": None},
            {"category": 35, "count": 0, "mode": None},
            {"category": 36, "count": 0, "mode": None},
            {"category": 37, "count": 0, "mode": None},
            {"category": 38, "count": 0, "mode": None},
            {"category": 39, "count": 1, "mode": 8},
            {"category": 40, "count": 2, "mode": 4},
            {"category": 41, "count": 0, "mode": None},
            {"category": 42, "count": 0, "mode": None},
            {"category": 43, "count": 1, "mode": 9},
            {"category": 44, "count": 0, "mode": None},
        ]

        result = json.loads(v_vect_stats.outputs.stdout)
        self.assert_json_equal(expected_results, result)


if __name__ == "__main__":
    test()
