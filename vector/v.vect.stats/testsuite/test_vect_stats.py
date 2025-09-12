"""
Name:       v.vect.stats test
Purpose:    Tests v.vect.stats and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

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


class Testrr(TestCase):
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
            count_column="num_points",
            stats_column="avg_elev",
            points_column="cat",
            flags="p",
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
            count_column="num_points",
            stats_column="avg_elev",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output2, actual=v_vect_stats.outputs.stdout)

    def test_median(self):
        """Testing method variance"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="variance",
            count_column="num_points",
            stats_column="avg_elev",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output3, actual=v_vect_stats.outputs.stdout)

    def test_mincat(self):
        """Testing method min_cat"""
        v_vect_stats = SimpleModule(
            "v.vect.stats",
            points=self.input,
            areas=self.areas,
            method="range",
            count_column="num_points",
            stats_column="avg_elev",
            points_column="cat",
            flags="p",
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
            count_column="num_points",
            stats_column="avg_elev",
            points_column="cat",
            flags="p",
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
            count_column="num_points",
            stats_column="avg_elev",
            points_column="cat",
            flags="p",
        )
        self.assertModule(v_vect_stats)
        self.assertLooksLike(reference=output6, actual=v_vect_stats.outputs.stdout)


if __name__ == "__main__":
    test()
