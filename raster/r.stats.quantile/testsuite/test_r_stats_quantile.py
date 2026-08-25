import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

reference_str_1 = """27511:0:33.333333:134.717331
27511:1:66.666667:143.985499
27513:0:33.333333:140.669418
27513:1:66.666667:146.279022
27518:0:33.333333:115.140050
27518:1:66.666667:129.893631
27529:0:33.333333:96.325943
27529:1:66.666667:103.571231
27539:0:33.333333:123.216911
27539:1:66.666667:133.431442
27601:0:33.333333:89.850876
27601:1:66.666667:98.379255
27603:0:33.333333:91.602473
27603:1:66.666667:103.511017
27604:0:33.333333:75.695976
27604:1:66.666667:87.048233
27605:0:33.333333:102.274857
27605:1:66.666667:112.512177
27606:0:33.333333:108.092461
27606:1:66.666667:124.441594
27607:0:33.333333:124.740369
27607:1:66.666667:135.907516
27608:0:33.333333:91.426587
27608:1:66.666667:104.532201
27610:0:33.333333:81.664024
27610:1:66.666667:92.704079"""

reference_str_2 = """27511:0:50.000000:139.625984
27511:1:60.000000:142.374435
27511:2:70.000000:144.753278
27513:0:50.000000:143.704910
27513:1:60.000000:145.188040
27513:2:70.000000:146.847707
27518:0:50.000000:122.534378
27518:1:60.000000:126.608752
27518:2:70.000000:131.621115
27529:0:50.000000:100.000465
27529:1:60.000000:102.160379
27529:2:70.000000:104.277072
27539:0:50.000000:128.539337
27539:1:60.000000:131.724411
27539:2:70.000000:134.259715
27601:0:50.000000:94.191139
27601:1:60.000000:96.736372
27601:2:70.000000:99.186425
27603:0:50.000000:97.599670
27603:1:60.000000:101.081412
27603:2:70.000000:104.792902
27604:0:50.000000:82.088058
27604:1:60.000000:85.149146
27604:2:70.000000:87.923511
27605:0:50.000000:108.045738
27605:1:60.000000:110.635651
27605:2:70.000000:113.597951
27606:0:50.000000:116.578697
27606:1:60.000000:121.248445
27606:2:70.000000:126.099172
27607:0:50.000000:130.293945
27607:1:60.000000:133.615195
27607:2:70.000000:136.992752
27608:0:50.000000:97.371792
27608:1:60.000000:101.015968
27608:2:70.000000:106.343367
27610:0:50.000000:87.330147
27610:1:60.000000:90.682228
27610:2:70.000000:93.667923
"""

reference_str_3 = """cat:50.000000:60.000000:70.000000
27511:139.625984:142.374435:144.753278
27513:143.704910:145.188040:146.847707
27518:122.534378:126.608752:131.621115
27529:100.000465:102.160379:104.277072
27539:128.539337:131.724411:134.259715
27601:94.191139:96.736372:99.186425
27603:97.599670:101.081412:104.792902
27604:82.088058:85.149146:87.923511
27605:108.045738:110.635651:113.597951
27606:116.578697:121.248445:126.099172
27607:130.293945:133.615195:136.992752
27608:97.371792:101.015968:106.343367
27610:87.330147:90.682228:93.667923
"""


class TestStatsQuantile(TestCase):
    base = "zipcodes"
    cover = "elevation"

    def setUp(self):
        self.use_temp_region()
        self.runModule("g.region", raster="elevation")

    def tearDown(self):
        self.del_temp_region()

    def test_quantiles_default(self):
        """Test quantiles with default settings."""
        module = SimpleModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            quantiles=3,
            flags="p",
        )
        self.assertModule(module)
        self.assertLooksLike(
            actual=str(module.outputs.stdout), reference=reference_str_1
        )

    def test_plain_format(self):
        """Test flag p and plain format."""
        module = SimpleModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            percentiles=["50", "60", "70"],
            flags="p",
        )
        self.assertModule(module)
        self.assertLooksLike(
            actual=str(module.outputs.stdout), reference=reference_str_2
        )

        # Explicitly test plain format
        module = SimpleModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            percentiles=["50", "60", "70"],
            flags="p",
            format="plain",
        )
        self.assertModule(module)
        self.assertLooksLike(
            actual=str(module.outputs.stdout), reference=reference_str_2
        )

    def test_table_format(self):
        """Test csv format and t flag."""
        module = SimpleModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            percentiles=["50", "60", "70"],
            flags="t",
        )
        self.assertModule(module)
        self.assertLooksLike(
            actual=str(module.outputs.stdout), reference=reference_str_3
        )

        # CSV format with flag p
        module = SimpleModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            percentiles=["50", "60", "70"],
            flags="p",
            format="csv",
        )
        self.assertModule(module)
        self.assertLooksLike(
            actual=str(module.outputs.stdout),
            reference=reference_str_3.replace(":", ","),
        )

        # CSV format with t flag
        module = SimpleModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            percentiles=["50", "60", "70"],
            flags="t",
            format="csv",
        )
        self.assertModule(module)
        self.assertLooksLike(
            actual=str(module.outputs.stdout),
            reference=reference_str_3.replace(":", ","),
        )

    def test_json_format(self):
        """Test JSON output format."""
        module = SimpleModule(
            "r.stats.quantile",
            base=self.base,
            cover=self.cover,
            percentiles=["60", "70"],
            flags="p",
            format="json",
        )
        self.assertModule(module)

        expected = [
            {
                "category": 27511,
                "percentiles": [
                    {"percentile": 60, "value": 142.3744354248047},
                    {"percentile": 70, "value": 144.75327758789064},
                ],
            },
            {
                "category": 27513,
                "percentiles": [
                    {"percentile": 60, "value": 145.18804016113282},
                    {"percentile": 70, "value": 146.8477066040039},
                ],
            },
            {
                "category": 27518,
                "percentiles": [
                    {"percentile": 60, "value": 126.60875244140624},
                    {"percentile": 70, "value": 131.6211151123047},
                ],
            },
            {
                "category": 27529,
                "percentiles": [
                    {"percentile": 60, "value": 102.16037902832032},
                    {"percentile": 70, "value": 104.27707214355469},
                ],
            },
            {
                "category": 27539,
                "percentiles": [
                    {"percentile": 60, "value": 131.7244110107422},
                    {"percentile": 70, "value": 134.25971527099608},
                ],
            },
            {
                "category": 27601,
                "percentiles": [
                    {"percentile": 60, "value": 96.73637237548829},
                    {"percentile": 70, "value": 99.18642501831054},
                ],
            },
            {
                "category": 27603,
                "percentiles": [
                    {"percentile": 60, "value": 101.08141174316407},
                    {"percentile": 70, "value": 104.79290161132812},
                ],
            },
            {
                "category": 27604,
                "percentiles": [
                    {"percentile": 60, "value": 85.1491455078125},
                    {"percentile": 70, "value": 87.9235107421875},
                ],
            },
            {
                "category": 27605,
                "percentiles": [
                    {"percentile": 60, "value": 110.63565063476562},
                    {"percentile": 70, "value": 113.5979507446289},
                ],
            },
            {
                "category": 27606,
                "percentiles": [
                    {"percentile": 60, "value": 121.24844512939453},
                    {"percentile": 70, "value": 126.09917221069337},
                ],
            },
            {
                "category": 27607,
                "percentiles": [
                    {"percentile": 60, "value": 133.61519470214844},
                    {"percentile": 70, "value": 136.9927520751953},
                ],
            },
            {
                "category": 27608,
                "percentiles": [
                    {"percentile": 60, "value": 101.0159683227539},
                    {"percentile": 70, "value": 106.34336700439452},
                ],
            },
            {
                "category": 27610,
                "percentiles": [
                    {"percentile": 60, "value": 90.6822280883789},
                    {"percentile": 70, "value": 93.66792297363281},
                ],
            },
        ]
        output_json = json.loads(module.outputs.stdout)

        self.assertEqual(len(expected), len(output_json))
        for exp_cat, out_cat in zip(expected, output_json, strict=True):
            self.assertEqual(exp_cat["category"], out_cat["category"])
            self.assertEqual(len(exp_cat["percentiles"]), len(out_cat["percentiles"]))
            for exp_p, out_p in zip(
                exp_cat["percentiles"], out_cat["percentiles"], strict=True
            ):
                self.assertAlmostEqual(
                    exp_p["percentile"], out_p["percentile"], places=6
                )
                self.assertAlmostEqual(exp_p["value"], out_p["value"], places=6)


if __name__ == "__main__":
    test()
