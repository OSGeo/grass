from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
import grass.script.core as gcore

import json
from itertools import zip_longest

# not used yet
LOCATION = "nc_spm"

output1 = """
 0.000000 88.370453
 10.000000 88.397057
 20.000000 89.526253
 30.000000 89.677551
 40.000000 91.297195
 50.000000 91.297195
 60.000000 92.330658
 70.000000 93.069199
 80.000000 94.768280
 90.000000 95.524551
 100.000000 96.770805
 110.000000 96.770805
 120.000000 97.418869
"""

output2 = """
637656.000000 224222.000000 0.000000 88.370453
637664.540486 224227.201932 10.000000 88.397057
637673.080972 224232.403865 20.000000 89.526253
637681.621458 224237.605797 30.000000 89.677551
637690.161944 224242.807729 40.000000 91.297195
637698.702430 224248.009662 50.000000 91.297195
637707.242916 224253.211594 60.000000 92.330658
637715.783402 224258.413526 70.000000 93.069199
637724.323887 224263.615459 80.000000 94.768280
637732.864373 224268.817391 90.000000 95.524551
637741.404859 224274.019323 100.000000 96.770805
637749.945345 224279.221256 110.000000 96.770805
637758.485831 224284.423188 120.000000 97.418869
"""

output3 = """
 0.000000 91.071831
 10.000000 91.431198
 20.000000 91.746628
 30.000000 91.746628
 40.000000 91.748047
 50.000000 91.872192
 60.000000 91.730049
 70.000000 91.690292
 80.000000 91.341331
 86.533231 91.341331
 96.533231 91.639000
 106.533231 nodata
 116.533231 nodata
 126.533231 nodata
 136.533231 nodata
 146.533231 nodata
 156.533231 nodata
 166.533231 nodata
 176.533231 nodata
 186.533231 nodata
 196.533231 nodata
 206.533231 nodata
 216.533231 nodata
"""

output4 = """
 0.000000 88.370453
 25.000000 89.526253
 50.000000 91.297195
 75.000000 94.768280
 100.000000 96.770805
 125.000000 97.646629
"""

output5 = """
635747.000000 222664.000000 0.000000 117.672462
635738.870095 222669.822770 10.000000 116.417213
635730.740190 222675.645539 20.000000 115.639481
635722.610285 222681.468309 30.000000 112.835342
635714.480381 222687.291079 40.000000 111.324890
635706.350476 222693.113848 50.000000 108.612282
635698.220571 222698.936618 60.000000 106.313347
635690.090666 222704.759388 70.000000 104.915665
635681.960761 222710.582158 80.000000 102.878601
635673.830856 222716.404927 90.000000 102.935074
635673.000000 222717.000000 91.021975 102.935074
635665.017450 222710.976803 101.021975 102.932213
635657.034900 222704.953607 111.021975 102.931152
635649.052351 222698.930410 121.021975 102.932213
635641.069801 222692.907213 131.021975 102.932213
635633.087251 222686.884017 141.021975 102.931648
635625.104701 222680.860820 151.021975 102.936768
635617.122151 222674.837623 161.021975 102.903358
635609.139601 222668.814427 171.021975 105.447823
635601.157052 222662.791230 181.021975 105.447823
635593.174502 222656.768033 191.021975 108.423523
635585.191952 222650.744836 201.021975 109.192360
635577.209402 222644.721640 211.021975 112.042763
635569.226852 222638.698443 221.021975 114.321136
635563.000000 222634.000000 228.822556 114.321136
635569.507914 222626.407434 238.822556 115.357292
635576.015827 222618.814868 248.822556 114.609100
635582.523741 222611.222302 258.822556 111.636292
635589.031655 222603.629736 268.822556 112.355431
635595.539569 222596.037170 278.822556 110.162842
635602.047482 222588.444604 288.822556 109.172668
635608.555396 222580.852038 298.822556 109.172668
635615.063310 222573.259472 308.822556 108.030540
635621.571224 222565.666906 318.822556 105.670113
635628.079137 222558.074340 328.822556 105.770287
635634.587051 222550.481774 338.822556 105.169937
635641.000000 222543.000000 348.676634 105.416862
635647.589446 222550.521915 358.676634 105.011185
635654.178892 222558.043830 368.676634 104.854263
635660.768338 222565.565744 378.676634 104.573921
635667.357784 222573.087659 388.676634 103.413361
635673.947230 222580.609574 398.676634 105.485588
635680.536676 222588.131489 408.676634 109.791016
635687.126122 222595.653403 418.676634 109.701485
635693.715568 222603.175318 428.676634 112.104370
635700.305014 222610.697233 438.676634 113.684036
635706.894460 222618.219148 448.676634 113.684036
635713.483906 222625.741062 458.676634 114.252579
635720.073352 222633.262977 468.676634 114.115379
635726.662798 222640.784892 478.676634 114.123955
635733.252244 222648.306807 488.676634 115.766998
635739.841690 222655.828721 498.676634 116.547440
635746.431136 222663.350636 508.676634 117.672462
"""

output_json_with_color = [
    {
        "easting": 635747,
        "northing": 222664,
        "distance": 0,
        "elevation": 117.67246246337891,
        "red": 250,
        "green": 127,
        "blue": 5,
    },
    {
        "easting": 635738.87009513902,
        "northing": 222669.8227696977,
        "distance": 10.000000000050347,
        "elevation": 116.41721343994141,
        "red": 254,
        "green": 127,
        "blue": 1,
    },
    {
        "easting": 635730.74019027804,
        "northing": 222675.64553939539,
        "distance": 20.000000000100695,
        "elevation": 115.63948059082031,
        "red": 255,
        "green": 130,
        "blue": 0,
    },
    {
        "easting": 635722.61028541706,
        "northing": 222681.46830909309,
        "distance": 30.00000000015104,
        "elevation": 112.83534240722656,
        "red": 255,
        "green": 148,
        "blue": 0,
    },
    {
        "easting": 635714.48038055608,
        "northing": 222687.29107879079,
        "distance": 40.000000000201389,
        "elevation": 111.32489013671875,
        "red": 255,
        "green": 157,
        "blue": 0,
    },
    {
        "easting": 635706.3504756951,
        "northing": 222693.11384848849,
        "distance": 50.000000000251738,
        "elevation": 108.61228179931641,
        "red": 255,
        "green": 175,
        "blue": 0,
    },
    {
        "easting": 635698.22057083412,
        "northing": 222698.93661818618,
        "distance": 60.000000000302087,
        "elevation": 106.31334686279297,
        "red": 255,
        "green": 189,
        "blue": 0,
    },
    {
        "easting": 635690.09066597314,
        "northing": 222704.75938788388,
        "distance": 70.000000000352429,
        "elevation": 104.91566467285156,
        "red": 255,
        "green": 198,
        "blue": 0,
    },
    {
        "easting": 635681.96076111216,
        "northing": 222710.58215758158,
        "distance": 80.000000000402778,
        "elevation": 102.87860107421875,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635673.83085625118,
        "northing": 222716.40492727928,
        "distance": 90.000000000453127,
        "elevation": 102.93507385253906,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635673,
        "northing": 222717,
        "distance": 91.021975368588883,
        "elevation": 102.93507385253906,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635665.01745017618,
        "northing": 222710.97680331473,
        "distance": 101.02197536855223,
        "elevation": 102.93221282958984,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635657.03490035236,
        "northing": 222704.95360662945,
        "distance": 111.02197536851558,
        "elevation": 102.93115234375,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635649.05235052854,
        "northing": 222698.93040994418,
        "distance": 121.02197536847893,
        "elevation": 102.93221282958984,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635641.06980070472,
        "northing": 222692.90721325891,
        "distance": 131.02197536844227,
        "elevation": 102.93221282958984,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635633.0872508809,
        "northing": 222686.88401657363,
        "distance": 141.02197536840561,
        "elevation": 102.93164825439453,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635625.10470105708,
        "northing": 222680.86081988836,
        "distance": 151.02197536836894,
        "elevation": 102.936767578125,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635617.12215123326,
        "northing": 222674.83762320309,
        "distance": 161.02197536833228,
        "elevation": 102.90335845947266,
        "red": 255,
        "green": 211,
        "blue": 0,
    },
    {
        "easting": 635609.13960140944,
        "northing": 222668.81442651781,
        "distance": 171.02197536829561,
        "elevation": 105.44782257080078,
        "red": 255,
        "green": 195,
        "blue": 0,
    },
    {
        "easting": 635601.15705158561,
        "northing": 222662.79122983254,
        "distance": 181.02197536825895,
        "elevation": 105.44782257080078,
        "red": 255,
        "green": 195,
        "blue": 0,
    },
    {
        "easting": 635593.17450176179,
        "northing": 222656.76803314727,
        "distance": 191.02197536822229,
        "elevation": 108.42352294921875,
        "red": 255,
        "green": 176,
        "blue": 0,
    },
    {
        "easting": 635585.19195193797,
        "northing": 222650.74483646199,
        "distance": 201.02197536818562,
        "elevation": 109.19235992431641,
        "red": 255,
        "green": 171,
        "blue": 0,
    },
    {
        "easting": 635577.20940211415,
        "northing": 222644.72163977672,
        "distance": 211.02197536814896,
        "elevation": 112.04276275634766,
        "red": 255,
        "green": 153,
        "blue": 0,
    },
    {
        "easting": 635569.22685229033,
        "northing": 222638.69844309145,
        "distance": 221.02197536811229,
        "elevation": 114.32113647460938,
        "red": 255,
        "green": 138,
        "blue": 0,
    },
    {
        "easting": 635563,
        "northing": 222634,
        "distance": 228.82255591888975,
        "elevation": 114.32113647460938,
        "red": 255,
        "green": 138,
        "blue": 0,
    },
    {
        "easting": 635569.50791373453,
        "northing": 222626.40743397636,
        "distance": 238.82255591886044,
        "elevation": 115.35729217529297,
        "red": 255,
        "green": 132,
        "blue": 0,
    },
    {
        "easting": 635576.01582746906,
        "northing": 222618.81486795272,
        "distance": 248.82255591883114,
        "elevation": 114.60910034179688,
        "red": 255,
        "green": 137,
        "blue": 0,
    },
    {
        "easting": 635582.52374120359,
        "northing": 222611.22230192908,
        "distance": 258.82255591880187,
        "elevation": 111.63629150390625,
        "red": 255,
        "green": 155,
        "blue": 0,
    },
    {
        "easting": 635589.03165493812,
        "northing": 222603.62973590544,
        "distance": 268.82255591877259,
        "elevation": 112.35543060302734,
        "red": 255,
        "green": 151,
        "blue": 0,
    },
    {
        "easting": 635595.53956867266,
        "northing": 222596.03716988181,
        "distance": 278.82255591874332,
        "elevation": 110.162841796875,
        "red": 255,
        "green": 165,
        "blue": 0,
    },
    {
        "easting": 635602.04748240719,
        "northing": 222588.44460385817,
        "distance": 288.82255591871404,
        "elevation": 109.17266845703125,
        "red": 255,
        "green": 171,
        "blue": 0,
    },
    {
        "easting": 635608.55539614172,
        "northing": 222580.85203783453,
        "distance": 298.82255591868477,
        "elevation": 109.17266845703125,
        "red": 255,
        "green": 171,
        "blue": 0,
    },
    {
        "easting": 635615.06330987625,
        "northing": 222573.25947181089,
        "distance": 308.8225559186555,
        "elevation": 108.03054046630859,
        "red": 255,
        "green": 178,
        "blue": 0,
    },
    {
        "easting": 635621.57122361078,
        "northing": 222565.66690578725,
        "distance": 318.82255591862622,
        "elevation": 105.67011260986328,
        "red": 255,
        "green": 193,
        "blue": 0,
    },
    {
        "easting": 635628.07913734531,
        "northing": 222558.07433976361,
        "distance": 328.82255591859695,
        "elevation": 105.77028656005859,
        "red": 255,
        "green": 193,
        "blue": 0,
    },
    {
        "easting": 635634.58705107984,
        "northing": 222550.48177373997,
        "distance": 338.82255591856767,
        "elevation": 105.16993713378906,
        "red": 255,
        "green": 196,
        "blue": 0,
    },
    {
        "easting": 635641,
        "northing": 222543,
        "distance": 348.67663386369753,
        "elevation": 105.41686248779297,
        "red": 255,
        "green": 195,
        "blue": 0,
    },
    {
        "easting": 635647.58944598527,
        "northing": 222550.52191475674,
        "distance": 358.67663386372163,
        "elevation": 105.01118469238281,
        "red": 255,
        "green": 197,
        "blue": 0,
    },
    {
        "easting": 635654.17889197054,
        "northing": 222558.04382951348,
        "distance": 368.67663386374574,
        "elevation": 104.85426330566406,
        "red": 255,
        "green": 198,
        "blue": 0,
    },
    {
        "easting": 635660.76833795581,
        "northing": 222565.56574427022,
        "distance": 378.67663386376984,
        "elevation": 104.57392120361328,
        "red": 255,
        "green": 200,
        "blue": 0,
    },
    {
        "easting": 635667.35778394109,
        "northing": 222573.08765902696,
        "distance": 388.67663386379394,
        "elevation": 103.41336059570312,
        "red": 255,
        "green": 208,
        "blue": 0,
    },
    {
        "easting": 635673.94722992636,
        "northing": 222580.6095737837,
        "distance": 398.67663386381804,
        "elevation": 105.48558807373047,
        "red": 255,
        "green": 194,
        "blue": 0,
    },
    {
        "easting": 635680.53667591163,
        "northing": 222588.13148854044,
        "distance": 408.67663386384214,
        "elevation": 109.791015625,
        "red": 255,
        "green": 167,
        "blue": 0,
    },
    {
        "easting": 635687.1261218969,
        "northing": 222595.65340329718,
        "distance": 418.67663386386624,
        "elevation": 109.70148468017578,
        "red": 255,
        "green": 168,
        "blue": 0,
    },
    {
        "easting": 635693.71556788217,
        "northing": 222603.17531805392,
        "distance": 428.67663386389034,
        "elevation": 112.1043701171875,
        "red": 255,
        "green": 152,
        "blue": 0,
    },
    {
        "easting": 635700.30501386744,
        "northing": 222610.69723281066,
        "distance": 438.67663386391445,
        "elevation": 113.68403625488281,
        "red": 255,
        "green": 142,
        "blue": 0,
    },
    {
        "easting": 635706.89445985272,
        "northing": 222618.2191475674,
        "distance": 448.67663386393855,
        "elevation": 113.68403625488281,
        "red": 255,
        "green": 142,
        "blue": 0,
    },
    {
        "easting": 635713.48390583799,
        "northing": 222625.74106232414,
        "distance": 458.67663386396265,
        "elevation": 114.25257873535156,
        "red": 255,
        "green": 139,
        "blue": 0,
    },
    {
        "easting": 635720.07335182326,
        "northing": 222633.26297708089,
        "distance": 468.67663386398675,
        "elevation": 114.11537933349609,
        "red": 255,
        "green": 140,
        "blue": 0,
    },
    {
        "easting": 635726.66279780853,
        "northing": 222640.78489183763,
        "distance": 478.67663386401085,
        "elevation": 114.12395477294922,
        "red": 255,
        "green": 140,
        "blue": 0,
    },
    {
        "easting": 635733.2522437938,
        "northing": 222648.30680659437,
        "distance": 488.67663386403495,
        "elevation": 115.76699829101562,
        "red": 255,
        "green": 129,
        "blue": 0,
    },
    {
        "easting": 635739.84168977907,
        "northing": 222655.82872135111,
        "distance": 498.67663386405906,
        "elevation": 116.54743957519531,
        "red": 254,
        "green": 127,
        "blue": 1,
    },
    {
        "easting": 635746.43113576435,
        "northing": 222663.35063610785,
        "distance": 508.67663386408316,
        "elevation": 117.67246246337891,
        "red": 250,
        "green": 127,
        "blue": 5,
    },
]


class TestProfileNCSPM(TestCase):
    @classmethod
    def setUpClass(cls):
        gcore.use_temp_region()
        gcore.run_command("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        gcore.del_temp_region()

    def test_profile_default(self):
        rprofile = SimpleModule(
            "r.profile", input="elevation", coordinates=[637656, 224222, 637766, 224289]
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output1.strip())
        self.assertIn("128.798292 [meters]", rprofile.outputs.stderr)  # distance
        self.assertIn("10 [meters]", rprofile.outputs.stderr)  # resolution

    def test_profile_m(self):
        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            units="meters",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertIn("128.798292 [meters]", rprofile.outputs.stderr)  # distance
        self.assertIn("10 [meters]", rprofile.outputs.stderr)  # resolution

    def test_profile_resolution(self):
        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            resolution=25,
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output4.strip())
        self.assertIn("128.798292 [meters]", rprofile.outputs.stderr)  # distance
        self.assertIn("25 [meters]", rprofile.outputs.stderr)  # resolution

    def test_profile_ne(self):
        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            flags="g",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output2.strip())

    def test_profile_region(self):
        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            null_value="nodata",
            coordinates=[644914, 224579, 644986, 224627, 645091, 224549],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output3.strip())
        self.assertIn(
            "WARNING: Endpoint coordinates are outside of current region settings",
            rprofile.outputs.stderr,
        )

    def test_profile_directions(self):
        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            flags="g",
            coordinates=[
                635747,
                222664,
                635673,
                222717,
                635563,
                222634,
                635641,
                222543,
                635747,
                222664,
            ],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output5.strip())

    def test_profile_json(self):
        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="g",
            format="json",
            coordinates=[
                635747,
                222664,
                635673,
                222717,
                635563,
                222634,
                635641,
                222543,
                635747,
                222664,
            ],
        )
        self.runModule(module)

        expected = []
        lines = output5.strip().split("\n")
        for line in lines:
            parts = line.split(" ")
            expected.append(
                {
                    "easting": float(parts[0]),
                    "northing": float(parts[1]),
                    "distance": float(parts[2]),
                    "elevation": float(parts[3]),
                }
            )
        result = json.loads(module.outputs.stdout)

        for entry1, entry2 in zip_longest(expected, result):
            self.assertAlmostEqual(entry1["easting"], entry2["easting"], places=6)
            self.assertAlmostEqual(entry1["northing"], entry2["northing"], places=6)
            self.assertAlmostEqual(entry1["distance"], entry2["distance"], places=6)
            self.assertAlmostEqual(entry1["elevation"], entry2["elevation"], places=6)

    def test_profile_json_color(self):
        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="gc",
            format="json",
            coordinates=[
                635747,
                222664,
                635673,
                222717,
                635563,
                222634,
                635641,
                222543,
                635747,
                222664,
            ],
        )
        self.runModule(module)

        expected = output_json_with_color
        result = json.loads(module.outputs.stdout)
        for entry1, entry2 in zip_longest(expected, result):
            self.assertAlmostEqual(entry1["easting"], entry2["easting"], places=6)
            self.assertAlmostEqual(entry1["northing"], entry2["northing"], places=6)
            self.assertAlmostEqual(entry1["distance"], entry2["distance"], places=6)
            self.assertAlmostEqual(entry1["elevation"], entry2["elevation"], places=6)
            self.assertEqual(entry1["red"], entry2["red"])
            self.assertEqual(entry1["blue"], entry2["blue"])
            self.assertEqual(entry1["green"], entry2["green"])


if __name__ == "__main__":
    test()
