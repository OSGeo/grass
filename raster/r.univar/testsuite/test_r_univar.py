"""Test of r.univar

@author Soeren Gebbert
"""

import json
from itertools import zip_longest

from grass.gunittest.case import TestCase

from grass.gunittest.gmodules import SimpleModule


class TestRasterUnivar(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name="map_a,map_b,map_negative,zone_map,zone_map_with_gap",
        )

    def setUp(self):
        """Create input data"""
        self.runModule("g.region", res=1, n=90, s=0, w=0, e=90)
        self.runModule(
            "r.mapcalc", expression="map_a = 100 + row() + col()", overwrite=True
        )
        self.runModule(
            "r.mapcalc", expression="map_b = 200 + row() + col()", overwrite=True
        )
        self.runModule(
            "r.mapcalc", expression="zone_map = if(row() < 20, 1, 2)", overwrite=True
        )
        self.runModule(
            "r.mapcalc",
            expression="zone_map_with_gap = if(row()> 20, 2, 9)",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="map_float = float(300) + row() + col()",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="map_double = double(400) + row() + col()",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="map_negative = -double(10) - row() - col()",
            overwrite=True,
        )

    def test_1(self):
        # Output of r.univar
        univar_string = """n=8100
        null_cells=0
        cells=8100
        min=102
        max=280
        range=178
        mean=191
        mean_of_abs=191
        sum=1547100"""

        self.assertRasterFitsUnivar(
            raster="map_a", reference=univar_string, precision=1e-10
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            format="shell",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_2(self):
        # Output of r.univar
        univar_string = """n=81
        null_cells=0
        cells=81
        min=112
        max=272
        range=160
        mean=192
        mean_of_abs=192
        sum=15552"""

        self.runModule("g.region", res=10)
        self.assertRasterFitsUnivar(
            raster="map_a", reference=univar_string, precision=1e-10
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            format="shell",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_3(self):
        """
        Check the -r flag
        :return:
        """

        univar_string = """n=8100
        null_cells=0
        cells=8100
        min=102
        max=280
        range=178
        mean=191
        mean_of_abs=191
        sum=1547100"""

        self.runModule("g.region", res=10)
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="rg",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="rg",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_extended(self):
        univar_string_float = """
        n=8100
        null_cells=0
        cells=8100
        min=302
        max=480
        range=178
        mean=391
        mean_of_abs=391
        stddev=36.7400780256838
        variance=1349.83333333333
        coeff_var=9.396439392758
        sum=3167100
        first_quartile=365
        median=391
        third_quartile=417
        percentile_90=441"""

        self.assertModuleKeyValue(
            module="r.univar",
            map="map_float",
            flags="ge",
            nprocs=1,
            reference=univar_string_float,
            precision=1e-10,
            sep="=",
        )

        univar_string_double = """
        n=8100
        null_cells=0
        cells=8100
        min=402
        max=580
        range=178
        mean=491
        mean_of_abs=491
        stddev=36.7400780256838
        variance=1349.83333333333
        coeff_var=7.48270428221666
        sum=3977100
        first_quartile=465
        median=491
        third_quartile=517
        percentile_90=541"""

        self.assertModuleKeyValue(
            module="r.univar",
            map="map_double",
            flags="ge",
            nprocs=1,
            reference=univar_string_double,
            precision=1e-10,
            sep="=",
        )

    def test_multiple_1(self):
        # Output of r.univar
        univar_string = """n=16200
        null_cells=0
        cells=16200
        min=102
        max=380
        range=278
        mean=241
        mean_of_abs=241
        sum=3904200"""

        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_multiple_2(self):
        # Output of r.univar
        univar_string = """n=162
        null_cells=0
        cells=162
        min=112
        max=372
        range=260
        mean=242
        mean_of_abs=242
        sum=39204"""

        self.runModule("g.region", res=10)
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="g",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            format="shell",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_multiple_3(self):
        """
        Check the -r flag
        :return:
        """

        # Output of r.univar
        univar_string = """n=16200
        null_cells=0
        cells=16200
        min=102
        max=380
        range=278
        mean=241
        mean_of_abs=241
        sum=3904200"""

        self.runModule("g.region", res=10)
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_negative(self):
        """
        check map with only negative values
        :return:
        """

        univar_string = """n=8100
        null_cells=0
        cells=8100
        min=-190
        max=-12
        range=178
        mean=-101
        mean_of_abs=101
        stddev=36.7400780256838
        variance=1349.83333333333
        coeff_var=-36.3763148769146
        sum=-818100"""

        self.runModule("g.region", res=10)
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_negative",
            flags="rg",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_negative",
            flags="rg",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_1_zone(self):
        """
        one map and zone
        :return:
        """

        # Output of r.univar
        univar_string = """zone=1;
                        n=1710
                        null_cells=0
                        cells=1710
                        min=102
                        max=209
                        range=107
                        mean=155.5
                        mean_of_abs=155.5
                        sum=265905
                        zone=2;
                        n=6390
                        null_cells=0
                        cells=6390
                        min=121
                        max=280
                        range=159
                        mean=200.5
                        mean_of_abs=200.5
                        sum=1281195"""

        self.runModule("g.region", res=1)
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a"],
            zones="zone_map",
            flags="g",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a"],
            zones="zone_map",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a"],
            zones="zone_map",
            format="shell",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_2_zone(self):
        """
        multiple maps and zone
        :return:
        """

        # Output of r.univar
        univar_string = """zone=1;
                        n=3420
                        null_cells=0
                        cells=3420
                        min=102
                        max=309
                        range=207
                        mean=205.5
                        mean_of_abs=205.5
                        stddev=56.6119834192962
                        variance=3204.91666666667
                        coeff_var=27.5484104230152
                        sum=702810
                        zone=2;
                        n=12780
                        null_cells=0
                        cells=12780
                        min=121
                        max=380
                        range=259
                        mean=250.5
                        mean_of_abs=250.5
                        stddev=59.9576239244574
                        variance=3594.91666666667
                        coeff_var=23.9351792113602
                        sum=3201390"""

        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="g",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_3_zone(self):
        """
        multiple maps and zone
        :return:
        """

        # Output of r.univar
        univar_string = """zone=1;
                        n=3420
                        null_cells=0
                        cells=3420
                        min=102
                        max=309
                        range=207
                        mean=205.5
                        mean_of_abs=205.5
                        stddev=56.6119834192962
                        variance=3204.91666666667
                        coeff_var=27.5484104230152
                        sum=702810
                        first_quartile=155
                        median=205.5
                        third_quartile=255
                        percentile_90=282
                        zone=2;
                        n=12780
                        null_cells=0
                        cells=12780
                        min=121
                        max=380
                        range=259
                        mean=250.5
                        mean_of_abs=250.5
                        stddev=59.9576239244574
                        variance=3594.91666666667
                        coeff_var=23.9351792113602
                        sum=3201390
                        first_quartile=200
                        median=250.5
                        third_quartile=300
                        percentile_90=330"""

        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="ge",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="ge",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_zone_with_gap_in_cats(self):
        """
        test if gaps in categories in a map are not included in the output
        :return:
        """

        # Output of r.univar
        univar_string = """zone=2;
                           n=12600
                           null_cells=0
                           cells=12600
                           min=122
                           max=380
                           range=258
                           mean=251
                           mean_of_abs=251
                           stddev=59.8595578555895
                           variance=3583.16666666667
                           coeff_var=23.8484294245376
                           sum=3162600
                           first_quartile=201
                           median=251
                           third_quartile=301
                           percentile_90=331
                           zone=9;
                           n=3600
                           null_cells=0
                           cells=3600
                           min=102
                           max=310
                           range=208
                           mean=206
                           mean_of_abs=206
                           stddev=56.6406803160649
                           variance=3208.16666666667
                           coeff_var=27.4954758815849
                           sum=741600
                           first_quartile=156
                           median=206
                           third_quartile=256
                           percentile_90=283"""

        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map_with_gap",
            flags="ge",
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map_with_gap",
            flags="ge",
            nprocs=4,
            reference=univar_string,
            precision=1e-10,
            sep="=",
        )

    def test_t_flag(self):
        reference = """
        zone|label|non_null_cells|null_cells|min|max|range|mean|mean_of_abs|stddev|variance|coeff_var|sum|sum_abs
1||1710|0|102|209|107|155.5|155.5|26.5502667908755|704.916666666667|17.0741265536177|265905|265905
2||6390|0|121|280|159|200.5|200.5|33.0895250293302|1094.91666666667|16.5035037552769|1281195|1281195
        """
        module = SimpleModule(
            "r.univar",
            map=["map_a"],
            zones="zone_map",
            flags="t",
        )
        self.runModule(module)
        self.assertEqual(module.outputs.stdout.strip(), reference.strip())

        # CSV takes precedence over the t flag
        reference = """
        zone,label,non_null_cells,null_cells,min,max,range,mean,mean_of_abs,stddev,variance,coeff_var,sum,sum_abs
1,,1710,0,102,209,107,155.5,155.5,26.5502667908755,704.916666666667,17.0741265536177,265905,265905
2,,6390,0,121,280,159,200.5,200.5,33.0895250293302,1094.91666666667,16.5035037552769,1281195,1281195
        """
        module = SimpleModule(
            "r.univar", map=["map_a"], zones="zone_map", flags="t", format="csv"
        )
        self.runModule(module)
        self.assertEqual(module.outputs.stdout.strip(), reference.strip())

        module = SimpleModule("r.univar", map=["map_a"], zones="zone_map", format="csv")
        self.runModule(module)
        self.assertEqual(module.outputs.stdout.strip(), reference.strip())

    def test_json(self):
        reference = {
            "n": 16200,
            "null_cells": 0,
            "cells": 16200,
            "min": 102,
            "max": 380,
            "range": 278,
            "mean": 241,
            "mean_of_abs": 241,
            "stddev": 62.04702517714555,
            "variance": 3849.8333333333335,
            "coeff_var": 25.745653600475332,
            "sum": 3904200,
            "first_quartile": 191,
            "median": 241,
            "third_quartile": 291,
            "percentiles": [{"percentile": 90, "value": 324}],
        }

        module = SimpleModule(
            "r.univar",
            map=["map_a", "map_b"],
            flags="e",
            format="json",
        )
        self.runModule(module)
        output = json.loads(module.outputs.stdout)

        for ref_key, ref_val in reference.items():
            if isinstance(ref_val, float):
                self.assertAlmostEqual(ref_val, output[ref_key], places=6)
            else:
                self.assertEqual(ref_val, output[ref_key])

    def test_json_zone(self):
        reference = [
            {
                "zone": 1,
                "zone_label": "",
                "n": 3420,
                "null_cells": 0,
                "cells": 3420,
                "min": 102,
                "max": 309,
                "range": 207,
                "mean": 205.5,
                "mean_of_abs": 205.5,
                "stddev": 56.611983419296187,
                "variance": 3204.9166666666665,
                "coeff_var": 27.548410423015174,
                "sum": 702810,
                "first_quartile": 155,
                "median": 205.5,
                "percentiles": [{"percentile": 90, "value": 282}],
                "third_quartile": 255,
            },
            {
                "zone": 2,
                "zone_label": "",
                "n": 12780,
                "null_cells": 0,
                "cells": 12780,
                "min": 121,
                "max": 380,
                "range": 259,
                "mean": 250.5,
                "mean_of_abs": 250.5,
                "stddev": 59.957623924457401,
                "variance": 3594.9166666666665,
                "coeff_var": 23.935179211360243,
                "sum": 3201390,
                "first_quartile": 200,
                "median": 250.5,
                "percentiles": [{"percentile": 90, "value": 330}],
                "third_quartile": 300,
            },
        ]

        module = SimpleModule(
            "r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="e",
            format="json",
        )
        self.runModule(module)
        output = json.loads(module.outputs.stdout)
        for expected, received in zip_longest(reference, output):
            self.assertCountEqual(list(expected.keys()), list(received.keys()))
            for key in expected:
                if isinstance(expected[key], float):
                    self.assertAlmostEqual(expected[key], received[key], places=6)
                else:
                    self.assertEqual(expected[key], received[key])


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
