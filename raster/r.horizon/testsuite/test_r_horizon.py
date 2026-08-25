"""
TEST:    test_r_horizon.py

AUTHOR(S): Anna Petrasova <kratochanna at gmail>

PURPOSE:   Test r.horizon

COPYRIGHT: (C) 2015-2024 Anna Petrasova

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import json

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script import raster_what


ref1 = """azimuth,horizon_height
180.000000,0.023101
"""

ref2 = """azimuth,horizon_height
180.000000,0.023101
200.000000,0.034850
220.000000,0.050549
240.000000,0.048211
260.000000,0.053101
280.000000,0.039774
300.000000,0.032360
320.000000,0.014804
340.000000,0.000000
360.000000,0.004724
20.000000,0.012612
40.000000,0.015207
60.000000,0.014344
80.000000,0.011044
100.000000,0.012192
120.000000,0.007462
140.000000,0.004071
160.000000,0.015356
"""

ref3 = """azimuth,horizon_height
180.000000,0.023101
200.000000,0.034850
220.000000,0.050549
240.000000,0.048211
260.000000,0.053101
280.000000,0.039774
300.000000,0.032360
320.000000,0.014804
340.000000,0.000000
360.000000,0.004724
20.000000,0.012612
40.000000,0.015207
60.000000,0.014344
80.000000,0.011044
100.000000,0.012192
120.000000,0.007462
140.000000,0.004071
160.000000,0.015356
"""

ref4 = """azimuth,horizon_height
0.000000,0.197017
20.000000,0.196832
40.000000,0.196875
60.000000,0.196689
80.000000,0.196847
100.000000,0.196645
120.000000,0.196969
140.000000,0.196778
160.000000,0.196863
180.000000,0.197017
200.000000,0.196832
220.000000,0.196875
240.000000,0.196689
260.000000,0.196847
280.000000,0.196645
300.000000,0.196969
320.000000,0.196778
340.000000,0.196863
"""

ref5 = """azimuth,horizon_height,horizon_distance
0.000000,0.197017,5000.040000
20.000000,0.196832,5004.837660
40.000000,0.196875,5003.728610
60.000000,0.196689,5008.552685
80.000000,0.196847,5004.448022
100.000000,0.196645,5009.690609
120.000000,0.196969,5001.279836
140.000000,0.196778,5006.246099
160.000000,0.196863,5004.018385
180.000000,0.197017,5000.040000
200.000000,0.196832,5004.837660
220.000000,0.196875,5003.728610
240.000000,0.196689,5008.552685
260.000000,0.196847,5004.448022
280.000000,0.196645,5009.690609
300.000000,0.196969,5001.279836
320.000000,0.196778,5006.246099
340.000000,0.196863,5004.018385
"""

ref6 = """azimuth,horizon_height,horizon_distance
180.000000,0.023101,420.000000
200.000000,0.034850,436.577599
220.000000,0.050549,184.390889
240.000000,0.048211,197.230829
260.000000,0.053101,162.788206
280.000000,0.039774,253.179778
300.000000,0.032360,277.848880
320.000000,0.014804,262.488095
340.000000,0.000000,0.000000
360.000000,0.004724,2780.017986
20.000000,0.012612,1148.259553
40.000000,0.015207,1334.166406
60.000000,0.014344,1867.966809
80.000000,0.011044,2964.203097
100.000000,0.012192,1828.223181
120.000000,0.007462,4270.667395
140.000000,0.004071,5659.231397
160.000000,0.015356,1666.883319
"""


class TestHorizon(TestCase):
    circle = "circle"
    horizon = "test_horizon_from_elevation"
    horizon_output = "test_horizon_output_from_elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")
        cls.runModule(
            "r.circle",
            flags="b",
            output=cls.circle,
            coordinates=(637505, 221755),
            min=5000,
            multiplier=1000,
        )
        cls.runModule("r.null", map=cls.circle, null=0)

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", flags="f", type="raster", name=cls.circle)
        cls.del_temp_region()

    def setUp(self):
        self.runModule("g.region", raster="elevation")

    def tearDown(self):
        """Remove horizon map after each test method"""
        self.runModule("g.remove", flags="f", type="raster", name=self.horizon)
        self.runModule(
            "g.remove", flags="f", type="raster", pattern=self.horizon_output + "*"
        )

    def test_point_mode_one_direction(self):
        """Test mode with 1 point and 1 direction"""
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            coordinates=(634720, 216180),
            output=self.horizon,
            direction=180,
            step=0,
        )
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(first=ref1, second=stdout)

    def test_point_mode_multiple_direction(self):
        """Test mode with 1 point and multiple directions"""
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            coordinates=(634720, 216180),
            output=self.horizon,
            direction=180,
            step=20,
        )
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(first=ref2, second=stdout)

        # include nulls along the edge
        self.runModule("g.region", raster="elevation", w="w-100")
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(first=ref2, second=stdout)

    def test_point_mode_multiple_points_and_directions(self):
        """Test mode with 2 identical points and multiple directions"""
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            coordinates=(634720, 216180, 634720, 216180),
            output=self.horizon,
            direction=180,
            step=20,
        )
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(first=ref2 + ref2, second=stdout)

    def test_point_mode_multiple_direction_json(self):
        """Test mode with 1 point and multiple directions with JSON"""
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            coordinates=(634720, 216180),
            output=self.horizon,
            direction=180,
            step=20,
            format="json",
        )
        self.assertModule(module)
        stdout = json.loads(module.outputs.stdout)
        horizons = []
        reference = {}
        for line in ref6.splitlines()[1:]:
            azimuth, horizon, distance = line.split(",")
            horizons.append(
                {
                    "azimuth": float(azimuth),
                    "angle": float(horizon),
                    "distance": float(distance),
                }
            )
        reference["x"] = 634720.0
        reference["y"] = 216180.0
        reference["horizons"] = horizons

        self.assertListEqual([reference], stdout)

    def test_point_mode_multiple_points_and_directions_json(self):
        """Test mode with 2 identical points and multiple directions with JSON"""
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            coordinates=(634720, 216180, 634720, 216180),
            output=self.horizon,
            direction=180,
            step=20,
            format="json",
        )
        self.assertModule(module)
        stdout = json.loads(module.outputs.stdout)
        horizons = []
        reference = {}
        for line in ref6.splitlines()[1:]:
            azimuth, horizon, distance = line.split(",")
            horizons.append(
                {
                    "azimuth": float(azimuth),
                    "angle": float(horizon),
                    "distance": float(distance),
                }
            )
        reference["x"] = 634720.0
        reference["y"] = 216180.0
        reference["horizons"] = horizons

        self.assertListEqual([reference, reference], stdout)

    def test_point_mode_multiple_direction_artificial(self):
        """Test mode with 1 point and multiple directions with artificial surface"""
        module = SimpleModule(
            "r.horizon",
            elevation=self.circle,
            coordinates=(637505, 221755),
            output=self.horizon,
            direction=0,
            step=20,
        )
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(first=ref4, second=stdout)

    def test_point_mode_multiple_direction_artificial_distance(self):
        """With 1 point, more directions on artificial surface, distance in output"""
        module = SimpleModule(
            "r.horizon",
            elevation=self.circle,
            coordinates=(637505, 221755),
            output=self.horizon,
            direction=0,
            step=20,
            flags="l",
        )
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(first=ref5, second=stdout)

        module = SimpleModule(
            "r.horizon",
            elevation=self.circle,
            coordinates=(637505, 221755),
            output=self.horizon,
            direction=0,
            step=20,
            flags="l",
            format="json",
        )
        self.assertModule(module)
        stdout = json.loads(module.outputs.stdout)
        horizons = []
        reference = {}
        for line in ref5.splitlines()[1:]:
            azimuth, horizon, distance = line.split(",")
            horizons.append(
                {
                    "azimuth": float(azimuth),
                    "angle": float(horizon),
                    "distance": float(distance),
                }
            )
        reference["x"] = 637505.0
        reference["y"] = 221755.0
        reference["horizons"] = horizons

        self.assertListEqual([reference], stdout)

    def test_raster_mode_one_direction(self):
        """Test mode with one direction and against point mode"""
        module = SimpleModule(
            "r.horizon", elevation="elevation", output=self.horizon_output, direction=50
        )
        self.assertModule(module)
        ref = {
            "min": 0,
            "max": 0.70678365230560,
            "stddev": 0.0360724286360789,
        }
        output = "test_horizon_output_from_elevation_050"
        self.assertRasterFitsUnivar(
            raster=output,
            reference=ref,
            precision=1e6,
        )

        # test if point mode matches raster mode
        coordinates = [
            (634725, 216185),
            (633315, 217595),
            (633555, 223405),
            (639955, 220605),
            (637505, 219705),
            (641105, 222225),
        ]
        for coordinate in coordinates:
            module = SimpleModule(
                "r.horizon",
                elevation="elevation",
                coordinates=coordinate,
                output=self.horizon,
                direction=50,
                step=0,
            )
            self.assertModule(module)
            stdout = module.outputs.stdout
            first = float(stdout.splitlines()[-1].split(",")[-1])
            what = raster_what(output, coord=coordinate)
            second = float(what[0][output]["value"])
            self.assertAlmostEqual(first=first, second=second, delta=0.000001)

    def test_raster_mode_multiple_direction(self):
        self.runModule("g.region", raster="elevation", res=100)
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            output=self.horizon_output,
            start=10,
            end=50,
            step=15.512,
        )
        self.assertModule(module)
        module_list = SimpleModule(
            "g.list", type="raster", pattern=self.horizon_output + "*"
        )
        self.runModule(module_list)
        stdout = module_list.outputs.stdout.strip()
        self.assertMultiLineEqual(
            first=(
                "test_horizon_output_from_elevation_010_000\n"
                "test_horizon_output_from_elevation_025_512\n"
                "test_horizon_output_from_elevation_041_024"
            ),
            second=stdout,
        )

    def test_raster_mode_multiple_direction_offset(self):
        self.runModule("g.region", raster="elevation", res=100)
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            output=self.horizon_output,
            start=10,
            end=50,
            step=15.512,
            direction=80,
        )
        self.assertModule(module)
        module_list = SimpleModule(
            "g.list", type="raster", pattern=self.horizon_output + "*"
        )
        self.runModule(module_list)
        stdout = module_list.outputs.stdout.strip()
        self.assertMultiLineEqual(
            first=(
                "test_horizon_output_from_elevation_090_000\n"
                "test_horizon_output_from_elevation_105_512\n"
                "test_horizon_output_from_elevation_121_024"
            ),
            second=stdout,
        )

    def test_raster_mode_bufferzone(self):
        """Test buffer 100 m and 109 m with resolution 10 gives the same result"""
        self.runModule(
            "g.region",
            raster="elevation",
            n="n-5000",
            s="s+5000",
            e="e-5000",
            w="w+5000",
        )
        # raises ValueError from pygrass parameter check
        self.assertRaises(
            ValueError,
            SimpleModule,
            "r.horizon",
            elevation="elevation",
            output=self.horizon_output,
            direction=50,
            bufferzone=-100,
        )
        self.assertRaises(
            ValueError,
            SimpleModule,
            "r.horizon",
            elevation="elevation",
            output=self.horizon_output,
            direction=50,
            e_buff=100,
            n_buff=0,
            s_buff=-100,
            w_buff=-100,
        )
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            output=self.horizon_output,
            direction=50,
            bufferzone=100,
        )
        self.assertModule(module)
        ref = {
            "mean": 0.0344791,
        }
        output = "test_horizon_output_from_elevation_050"
        self.assertRasterFitsUnivar(
            raster=output,
            reference=ref,
            precision=1e-6,
        )
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            output=self.horizon_output,
            direction=50,
            bufferzone=103,
        )
        self.assertModule(module)
        self.assertRasterFitsUnivar(
            raster=output,
            reference=ref,
            precision=1e-6,
        )
        module = SimpleModule(
            "r.horizon",
            elevation="elevation",
            output=self.horizon_output,
            direction=50,
            bufferzone=95,
        )
        self.assertModule(module)
        ref = {
            "mean": 0.0344624,
        }
        self.assertRasterFitsUnivar(
            raster=output,
            reference=ref,
            precision=1e-6,
        )
        self.runModule("g.region", raster="elevation")


if __name__ == "__main__":
    test()
