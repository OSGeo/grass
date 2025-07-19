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

output6 = """
 0.000000 88.370453 159:255:000
 10.000000 88.397057 160:255:000
 20.000000 89.526253 174:255:000
 30.000000 89.677551 176:255:000
 40.000000 91.297195 197:255:000
 50.000000 91.297195 197:255:000
 60.000000 92.330658 210:255:000
 70.000000 93.069199 219:255:000
 80.000000 94.768280 240:255:000
 90.000000 95.524551 250:255:000
 100.000000 96.770805 255:250:000
 110.000000 96.770805 255:250:000
 120.000000 97.418869 255:246:000
"""

output7 = """
 0.000000 88.370453 #9FFF00
 10.000000 88.397057 #A0FF00
 20.000000 89.526253 #AEFF00
 30.000000 89.677551 #B0FF00
 40.000000 91.297195 #C5FF00
 50.000000 91.297195 #C5FF00
 60.000000 92.330658 #D2FF00
 70.000000 93.069199 #DBFF00
 80.000000 94.768280 #F0FF00
 90.000000 95.524551 #FAFF00
 100.000000 96.770805 #FFFA00
 110.000000 96.770805 #FFFA00
 120.000000 97.418869 #FFF600
"""

output8 = """
 0.000000 88.370453 rgb(159, 255, 0)
 10.000000 88.397057 rgb(160, 255, 0)
 20.000000 89.526253 rgb(174, 255, 0)
 30.000000 89.677551 rgb(176, 255, 0)
 40.000000 91.297195 rgb(197, 255, 0)
 50.000000 91.297195 rgb(197, 255, 0)
 60.000000 92.330658 rgb(210, 255, 0)
 70.000000 93.069199 rgb(219, 255, 0)
 80.000000 94.768280 rgb(240, 255, 0)
 90.000000 95.524551 rgb(250, 255, 0)
 100.000000 96.770805 rgb(255, 250, 0)
 110.000000 96.770805 rgb(255, 250, 0)
 120.000000 97.418869 rgb(255, 246, 0)
"""

output9 = """
 0.000000 88.370453 hsv(82, 100, 100)
 10.000000 88.397057 hsv(82, 100, 100)
 20.000000 89.526253 hsv(79, 100, 100)
 30.000000 89.677551 hsv(78, 100, 100)
 40.000000 91.297195 hsv(73, 100, 100)
 50.000000 91.297195 hsv(73, 100, 100)
 60.000000 92.330658 hsv(70, 100, 100)
 70.000000 93.069199 hsv(68, 100, 100)
 80.000000 94.768280 hsv(63, 100, 100)
 90.000000 95.524551 hsv(61, 100, 100)
 100.000000 96.770805 hsv(58, 100, 100)
 110.000000 96.770805 hsv(58, 100, 100)
 120.000000 97.418869 hsv(57, 100, 100)
"""

output_json_with_color = [
    {
        "easting": 637656,
        "northing": 224222,
        "distance": 0,
        "value": 88.37045288085938,
        "color": "#9FFF00",
    },
    {
        "easting": 637664.5404859307,
        "northing": 224227.20193233964,
        "distance": 9.99999999997167,
        "value": 88.39705657958984,
        "color": "#A0FF00",
    },
    {
        "easting": 637673.0809718615,
        "northing": 224232.40386467928,
        "distance": 19.99999999994334,
        "value": 89.52625274658203,
        "color": "#AEFF00",
    },
    {
        "easting": 637681.6214577922,
        "northing": 224237.6057970189,
        "distance": 29.999999999915012,
        "value": 89.67755126953125,
        "color": "#B0FF00",
    },
    {
        "easting": 637690.161943723,
        "northing": 224242.80772935855,
        "distance": 39.99999999988668,
        "value": 91.29719543457031,
        "color": "#C5FF00",
    },
    {
        "easting": 637698.7024296537,
        "northing": 224248.0096616982,
        "distance": 49.99999999985835,
        "value": 91.29719543457031,
        "color": "#C5FF00",
    },
    {
        "easting": 637707.2429155845,
        "northing": 224253.21159403783,
        "distance": 59.999999999830024,
        "value": 92.33065795898438,
        "color": "#D2FF00",
    },
    {
        "easting": 637715.7834015152,
        "northing": 224258.41352637747,
        "distance": 69.9999999998017,
        "value": 93.06919860839844,
        "color": "#DBFF00",
    },
    {
        "easting": 637724.323887446,
        "northing": 224263.6154587171,
        "distance": 79.99999999977337,
        "value": 94.76828002929688,
        "color": "#F0FF00",
    },
    {
        "easting": 637732.8643733767,
        "northing": 224268.81739105674,
        "distance": 89.99999999974503,
        "value": 95.52455139160156,
        "color": "#FAFF00",
    },
    {
        "easting": 637741.4048593075,
        "northing": 224274.01932339638,
        "distance": 99.99999999971669,
        "value": 96.77080535888672,
        "color": "#FFFA00",
    },
    {
        "easting": 637749.9453452382,
        "northing": 224279.22125573602,
        "distance": 109.99999999968836,
        "value": 96.77080535888672,
        "color": "#FFFA00",
    },
    {
        "easting": 637758.485831169,
        "northing": 224284.42318807566,
        "distance": 119.99999999966002,
        "value": 97.41886901855469,
        "color": "#FFF600",
    },
]

output_json_with_color_rgb = [
    {
        "easting": 637656,
        "northing": 224222,
        "distance": 0,
        "value": 88.37045288085938,
        "color": "rgb(159, 255, 0)",
    },
    {
        "easting": 637664.5404859307,
        "northing": 224227.20193233964,
        "distance": 9.99999999997167,
        "value": 88.39705657958984,
        "color": "rgb(160, 255, 0)",
    },
    {
        "easting": 637673.0809718615,
        "northing": 224232.40386467928,
        "distance": 19.99999999994334,
        "value": 89.52625274658203,
        "color": "rgb(174, 255, 0)",
    },
    {
        "easting": 637681.6214577922,
        "northing": 224237.6057970189,
        "distance": 29.999999999915012,
        "value": 89.67755126953125,
        "color": "rgb(176, 255, 0)",
    },
    {
        "easting": 637690.161943723,
        "northing": 224242.80772935855,
        "distance": 39.99999999988668,
        "value": 91.29719543457031,
        "color": "rgb(197, 255, 0)",
    },
    {
        "easting": 637698.7024296537,
        "northing": 224248.0096616982,
        "distance": 49.99999999985835,
        "value": 91.29719543457031,
        "color": "rgb(197, 255, 0)",
    },
    {
        "easting": 637707.2429155845,
        "northing": 224253.21159403783,
        "distance": 59.999999999830024,
        "value": 92.33065795898438,
        "color": "rgb(210, 255, 0)",
    },
    {
        "easting": 637715.7834015152,
        "northing": 224258.41352637747,
        "distance": 69.9999999998017,
        "value": 93.06919860839844,
        "color": "rgb(219, 255, 0)",
    },
    {
        "easting": 637724.323887446,
        "northing": 224263.6154587171,
        "distance": 79.99999999977337,
        "value": 94.76828002929688,
        "color": "rgb(240, 255, 0)",
    },
    {
        "easting": 637732.8643733767,
        "northing": 224268.81739105674,
        "distance": 89.99999999974503,
        "value": 95.52455139160156,
        "color": "rgb(250, 255, 0)",
    },
    {
        "easting": 637741.4048593075,
        "northing": 224274.01932339638,
        "distance": 99.99999999971669,
        "value": 96.77080535888672,
        "color": "rgb(255, 250, 0)",
    },
    {
        "easting": 637749.9453452382,
        "northing": 224279.22125573602,
        "distance": 109.99999999968836,
        "value": 96.77080535888672,
        "color": "rgb(255, 250, 0)",
    },
    {
        "easting": 637758.485831169,
        "northing": 224284.42318807566,
        "distance": 119.99999999966002,
        "value": 97.41886901855469,
        "color": "rgb(255, 246, 0)",
    },
]

output_json_with_color_triplet = [
    {
        "easting": 637656,
        "northing": 224222,
        "distance": 0,
        "value": 88.37045288085938,
        "color": "159:255:0",
    },
    {
        "easting": 637664.5404859307,
        "northing": 224227.20193233964,
        "distance": 9.99999999997167,
        "value": 88.39705657958984,
        "color": "160:255:0",
    },
    {
        "easting": 637673.0809718615,
        "northing": 224232.40386467928,
        "distance": 19.99999999994334,
        "value": 89.52625274658203,
        "color": "174:255:0",
    },
    {
        "easting": 637681.6214577922,
        "northing": 224237.6057970189,
        "distance": 29.999999999915012,
        "value": 89.67755126953125,
        "color": "176:255:0",
    },
    {
        "easting": 637690.161943723,
        "northing": 224242.80772935855,
        "distance": 39.99999999988668,
        "value": 91.29719543457031,
        "color": "197:255:0",
    },
    {
        "easting": 637698.7024296537,
        "northing": 224248.0096616982,
        "distance": 49.99999999985835,
        "value": 91.29719543457031,
        "color": "197:255:0",
    },
    {
        "easting": 637707.2429155845,
        "northing": 224253.21159403783,
        "distance": 59.999999999830024,
        "value": 92.33065795898438,
        "color": "210:255:0",
    },
    {
        "easting": 637715.7834015152,
        "northing": 224258.41352637747,
        "distance": 69.9999999998017,
        "value": 93.06919860839844,
        "color": "219:255:0",
    },
    {
        "easting": 637724.323887446,
        "northing": 224263.6154587171,
        "distance": 79.99999999977337,
        "value": 94.76828002929688,
        "color": "240:255:0",
    },
    {
        "easting": 637732.8643733767,
        "northing": 224268.81739105674,
        "distance": 89.99999999974503,
        "value": 95.52455139160156,
        "color": "250:255:0",
    },
    {
        "easting": 637741.4048593075,
        "northing": 224274.01932339638,
        "distance": 99.99999999971669,
        "value": 96.77080535888672,
        "color": "255:250:0",
    },
    {
        "easting": 637749.9453452382,
        "northing": 224279.22125573602,
        "distance": 109.99999999968836,
        "value": 96.77080535888672,
        "color": "255:250:0",
    },
    {
        "easting": 637758.485831169,
        "northing": 224284.42318807566,
        "distance": 119.99999999966002,
        "value": 97.41886901855469,
        "color": "255:246:0",
    },
]

output_json_with_color_hsv = [
    {
        "easting": 637656,
        "northing": 224222,
        "distance": 0,
        "value": 88.37045288085938,
        "color": "hsv(82, 100, 100)",
    },
    {
        "easting": 637664.5404859307,
        "northing": 224227.20193233964,
        "distance": 9.99999999997167,
        "value": 88.39705657958984,
        "color": "hsv(82, 100, 100)",
    },
    {
        "easting": 637673.0809718615,
        "northing": 224232.40386467928,
        "distance": 19.99999999994334,
        "value": 89.52625274658203,
        "color": "hsv(79, 100, 100)",
    },
    {
        "easting": 637681.6214577922,
        "northing": 224237.6057970189,
        "distance": 29.999999999915012,
        "value": 89.67755126953125,
        "color": "hsv(78, 100, 100)",
    },
    {
        "easting": 637690.161943723,
        "northing": 224242.80772935855,
        "distance": 39.99999999988668,
        "value": 91.29719543457031,
        "color": "hsv(73, 100, 100)",
    },
    {
        "easting": 637698.7024296537,
        "northing": 224248.0096616982,
        "distance": 49.99999999985835,
        "value": 91.29719543457031,
        "color": "hsv(73, 100, 100)",
    },
    {
        "easting": 637707.2429155845,
        "northing": 224253.21159403783,
        "distance": 59.999999999830024,
        "value": 92.33065795898438,
        "color": "hsv(70, 100, 100)",
    },
    {
        "easting": 637715.7834015152,
        "northing": 224258.41352637747,
        "distance": 69.9999999998017,
        "value": 93.06919860839844,
        "color": "hsv(68, 100, 100)",
    },
    {
        "easting": 637724.323887446,
        "northing": 224263.6154587171,
        "distance": 79.99999999977337,
        "value": 94.76828002929688,
        "color": "hsv(63, 100, 100)",
    },
    {
        "easting": 637732.8643733767,
        "northing": 224268.81739105674,
        "distance": 89.99999999974503,
        "value": 95.52455139160156,
        "color": "hsv(61, 100, 100)",
    },
    {
        "easting": 637741.4048593075,
        "northing": 224274.01932339638,
        "distance": 99.99999999971669,
        "value": 96.77080535888672,
        "color": "hsv(58, 100, 100)",
    },
    {
        "easting": 637749.9453452382,
        "northing": 224279.22125573602,
        "distance": 109.99999999968836,
        "value": 96.77080535888672,
        "color": "hsv(58, 100, 100)",
    },
    {
        "easting": 637758.485831169,
        "northing": 224284.42318807566,
        "distance": 119.99999999966002,
        "value": 97.41886901855469,
        "color": "hsv(57, 100, 100)",
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

    def test_profile_color(self):
        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            flags="c",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output6.strip())

        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            flags="c",
            color_format="triplet",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output6.strip())

        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            flags="c",
            color_format="hex",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output7.strip())

        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            flags="c",
            color_format="rgb",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output8.strip())

        rprofile = SimpleModule(
            "r.profile",
            input="elevation",
            flags="c",
            color_format="hsv",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output9.strip())

    def _assert_json_equal(self, expected, result, has_color=True):
        for entry1, entry2 in zip_longest(expected, result):
            self.assertAlmostEqual(entry1["easting"], entry2["easting"], places=6)
            self.assertAlmostEqual(entry1["northing"], entry2["northing"], places=6)
            self.assertAlmostEqual(entry1["distance"], entry2["distance"], places=6)
            self.assertAlmostEqual(entry1["value"], entry2["value"], places=6)
            if has_color:
                self.assertEqual(entry1["color"], entry2["color"])

    def test_profile_json(self):
        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="g",
            format="json",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.runModule(module)
        self.assertEqual("", module.outputs.stderr)

        expected = []
        lines = output2.strip().split("\n")
        for line in lines:
            parts = line.split(" ")
            expected.append(
                {
                    "easting": float(parts[0]),
                    "northing": float(parts[1]),
                    "distance": float(parts[2]),
                    "value": float(parts[3]),
                }
            )
        result = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected, result, has_color=False)

    def test_profile_json_color(self):
        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="gc",
            format="json",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.runModule(module)
        self.assertEqual("", module.outputs.stderr)

        expected = output_json_with_color
        result = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected, result)

        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="gc",
            format="json",
            color_format="hex",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.runModule(module)
        self.assertEqual("", module.outputs.stderr)

        expected = output_json_with_color
        result = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected, result)

        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="gc",
            format="json",
            color_format="rgb",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.runModule(module)
        self.assertEqual("", module.outputs.stderr)

        expected = output_json_with_color_rgb
        result = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected, result)

        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="gc",
            format="json",
            color_format="triplet",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.runModule(module)
        self.assertEqual("", module.outputs.stderr)

        expected = output_json_with_color_triplet
        result = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected, result)

        module = SimpleModule(
            "r.profile",
            input="elevation",
            flags="gc",
            format="json",
            color_format="hsv",
            coordinates=[637656, 224222, 637766, 224289],
        )
        self.runModule(module)
        self.assertEqual("", module.outputs.stderr)

        expected = output_json_with_color_hsv
        result = json.loads(module.outputs.stdout)
        self._assert_json_equal(expected, result)


if __name__ == "__main__":
    test()
