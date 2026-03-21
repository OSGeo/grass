"""
Name:       r.geomorphon tests
Purpose:    Tests r.geomorphon input parsing.
            Uses NC Basic data set.

Author:     Luca Delucchi, Markus Neteler
Copyright:  (C) 2017 by Luca Delucchi, Markus Neteler and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command
import json

synth_out = """1	flat
3	ridge
4	shoulder
6	slope
8	footslope
9	valley
"""

ele_out = """1	flat
2	peak
3	ridge
4	shoulder
5	spur
6	slope
7	hollow
8	footslope
9	valley
10	pit
"""

expected_json = """{
    "search_rel_elevation_m": {
        "NE": {
            "step_1": 0.4113616943359375,
            "step_2": 0.469268798828125,
            "step_3": 0.37139892578125,
            "step_4": 0.02008056640625,
            "step_5": -0.668243408203125,
            "step_6": -1.13372802734375,
            "step_7": -1.169464111328125
        },
        "N": {
            "step_1": 0.0505523681640625,
            "step_2": 0.0650177001953125,
            "step_3": 0.03802490234375,
            "step_4": -0.080657958984375,
            "step_5": -0.2371063232421875,
            "step_6": -0.4404449462890625,
            "step_7": -0.75274658203125,
            "step_8": -1.202484130859375,
            "step_9": -1.6497650146484375
        },
        "NW": {
            "step_1": -0.415985107421875,
            "step_2": -0.94427490234375,
            "step_3": -1.5065765380859375,
            "step_4": -1.9257965087890625,
            "step_5": -2.231353759765625,
            "step_6": -2.38800048828125,
            "step_7": -2.4053955078125
        },
        "W": {
            "step_1": -0.4982757568359375,
            "step_2": -1.0782012939453125,
            "step_3": -1.780120849609375,
            "step_4": -2.5660858154296875,
            "step_5": -3.38287353515625,
            "step_6": -3.966766357421875,
            "step_7": -4.3426055908203125,
            "step_8": -4.7401580810546875,
            "step_9": -5.01177978515625
        },
        "SW": {
            "step_1": -0.638885498046875,
            "step_2": -1.2663726806640625,
            "step_3": -1.869293212890625,
            "step_4": -2.494903564453125,
            "step_5": -3.1828765869140625,
            "step_6": -4.0766754150390625,
            "step_7": -5.00653076171875
        },
        "S": {
            "step_1": -0.1096038818359375,
            "step_2": -0.2566986083984375,
            "step_3": -0.3684539794921875,
            "step_4": -0.4477691650390625,
            "step_5": -0.5353546142578125,
            "step_6": -0.668304443359375,
            "step_7": -0.9663238525390625,
            "step_8": -1.4843902587890625,
            "step_9": -2.102386474609375
        },
        "SE": {
            "step_1": 0.3142547607421875,
            "step_2": 0.62548828125,
            "step_3": 0.9405975341796875,
            "step_4": 1.1982269287109375,
            "step_5": 1.5157318115234375,
            "step_6": 1.7929229736328125,
            "step_7": 1.9519195556640625
        },
        "E": {
            "step_1": 0.3773956298828125,
            "step_2": 0.6216278076171875,
            "step_3": 0.7525177001953125,
            "step_4": 0.912353515625,
            "step_5": 0.994720458984375,
            "step_6": 1.0386962890625,
            "step_7": 1.1361236572265625,
            "step_8": 1.333770751953125,
            "step_9": 1.5446319580078125
        }
    },
    "map_info": {
        "elevation_name": "elevation",
        "projection": 99,
        "north": 228500,
        "south": 215000,
        "east": 645000,
        "west": 630000,
        "rows": 1350,
        "cols": 1500,
        "ewres": 10,
        "nsres": 10
    },
    "computation_parameters": {
        "easting": 636400,
        "northing": 221100,
        "search_m": 100,
        "search_cells": 10,
        "skip_m": 0,
        "skip_cells": 0,
        "flat_thresh_deg": 1,
        "flat_distance_m": 0,
        "flat_height_m": 0,
        "extended_correction": false
    },
    "intermediate_data": {
        "ternary_498": 26,
        "ternary_6561": 5834,
        "pattern_size": 8,
        "origin_easting": 636405,
        "origin_northing": 221095,
        "origin_elevation_m": 133.67021179199219,
        "num_positives": 3,
        "num_negatives": 5,
        "pattern": {
            "NE": 1,
            "N": -1,
            "NW": -1,
            "W": -1,
            "SW": -1,
            "S": -1,
            "SE": 1,
            "E": 1
        },
        "rel_elevation_m": {
            "NE": 0.4113616943359375,
            "N": -1.6497650146484375,
            "NW": -1.5065765380859375,
            "W": -3.38287353515625,
            "SW": -5.00653076171875,
            "S": -2.102386474609375,
            "SE": 0.3142547607421875,
            "E": 0.3773956298828125
        },
        "abs_elevation_m": {
            "NE": 134.08157348632812,
            "N": 132.02044677734375,
            "NW": 132.16363525390625,
            "W": 130.28733825683594,
            "SW": 128.66368103027344,
            "S": 131.56782531738281,
            "SE": 133.98446655273438,
            "E": 134.047607421875
        },
        "distance_m": {
            "NE": 14.142135623730951,
            "N": 90,
            "NW": 42.426406871192853,
            "W": 50,
            "SW": 98.994949366116657,
            "S": 90,
            "SE": 14.142135623730951,
            "E": 10
        },
        "offset_easting_m": {
            "NE": 10.000000000190248,
            "N": 0,
            "NW": -30.000000000570743,
            "W": -50,
            "SW": -70.000000001331728,
            "S": 0,
            "SE": 10.000000000190248,
            "E": 10
        },
        "offset_northing_m": {
            "NE": 10.000000000190248,
            "N": 90,
            "NW": 30.000000000570743,
            "W": 0,
            "SW": -70.000000001331728,
            "S": -90,
            "SE": -10.000000000190248,
            "E": 0
        },
        "easting": {
            "NE": 636415,
            "N": 636405,
            "NW": 636375,
            "W": 636355,
            "SW": 636335,
            "S": 636405,
            "SE": 636415,
            "E": 636415
        },
        "northing": {
            "NE": 221105,
            "N": 221185,
            "NW": 221125,
            "W": 221095,
            "SW": 221025,
            "S": 221005,
            "SE": 221085,
            "E": 221095
        }
    },
    "final_results": {
        "landform_cat": 5,
        "landform_code": "SP",
        "landform_name": "spur",
        "landform_deviation": 0,
        "azimuth": 63.434947967529297,
        "elongation": 2.0526316165924072,
        "width_m": 84.9705810546875,
        "intensity_m": 1.5681400299072266,
        "exposition_m": 5.00653076171875,
        "range_m": 5.4178924560546875,
        "variance": 3.3323256969451904,
        "extends": 0.28284271247461901,
        "octagon_perimeter_m": 429.9849048337291,
        "octagon_area_m2": 8000,
        "mesh_perimeter_m": 430.17266121004559,
        "mesh_area_m2": 8013.004939010184
    }
}"""


class TestClipling(TestCase):
    inele = "elevation"
    insint = "synthetic_dem"
    outele = "ele_geomorph"
    outsint = "synth_geomorph"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and generated data"""
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.inele)
        cls.runModule(
            "r.mapcalc",
            expression="{ou} = sin(x() / 5.0) + (sin(x() / 5.0) * 100.0 + 200)".format(
                ou=cls.insint
            ),
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=(cls.insint, cls.outele, cls.outsint),
        )
        cls.del_temp_region()

    def test_ele(self):
        self.runModule(
            "r.geomorphon", elevation=self.inele, forms=self.outele, search=10
        )
        category = read_command("r.category", map=self.outele)
        self.assertEqual(first=ele_out, second=category)

    def test_sint(self):
        self.runModule(
            "r.geomorphon", elevation=self.insint, forms=self.outsint, search=10
        )
        category = read_command("r.category", map=self.outsint)
        self.assertEqual(first=synth_out, second=category)

    def test_json_profile(self):
        """Test JSON profile output structure and key values"""

        out = read_command(
            "r.geomorphon",
            elevation=self.inele,
            coordinates="636400,221100",
            profiledata="-",
            profileformat="json",
            search=10,
        )

        try:
            json_out = json.loads(out)
        except ValueError:
            self.fail(msg="r.geomorphon produced invalid JSON:\n" + out)

        expected = json.loads(expected_json)
        for block in expected:
            if isinstance(expected[block], dict):
                for key in expected[block]:
                    if isinstance(expected[block][key], dict):
                        for sub_key in expected[block][key]:
                            if isinstance(expected[block][key][sub_key], float):
                                self.assertAlmostEqual(
                                    expected[block][key][sub_key],
                                    json_out[block][key][sub_key],
                                    places=4,
                                )
                            else:
                                self.assertEqual(
                                    expected[block][key][sub_key],
                                    json_out[block][key][sub_key],
                                )
                    elif isinstance(expected[block][key], float):
                        self.assertAlmostEqual(
                            expected[block][key], json_out[block][key], places=4
                        )
                    else:
                        self.assertEqual(expected[block][key], json_out[block][key])
            else:
                self.assertEqual(expected[block], json_out[block])


if __name__ == "__main__":
    test()
