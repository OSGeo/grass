"""Test the JSON extension of the GRASS parser

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

import subprocess
from grass.gunittest.case import TestCase
from grass.script import decode
import json


class TestParserJson(TestCase):
    def test_r_slope_aspect_json(self):
        args = ["r.slope.aspect",
                "elevation=elevation+https://storage.googleapis.com/graas-geodata/elev_ned_30m.tif",
                "slope=slope+GTiff",
                "aspect=aspect+GTiff", "--json"]

        inputs = [
            {"import_descr": {"source": "https://storage.googleapis.com/graas-geodata/elev_ned_30m.tif",
                              "type": "raster"},
             "param": "elevation", "value": "elevation"},
            {"param": "format", "value": "degrees"},
            {"param": "precision", "value": "FCELL"},
            {"param": "zscale", "value": "1.0"},
            {"param": "min_slope", "value": "0.0"}
        ]

        outputs = [
            {"export": {"format": "GTiff", "type": "raster"},
             "param": "slope", "value": "slope"},
            {"export": {"format": "GTiff", "type": "raster"},
             "param": "aspect", "value": "aspect"}
        ]

        stdout, stderr = subprocess.Popen(args, stdout=subprocess.PIPE).communicate()
        print(stdout)
        json_code = json.loads(decode(stdout))
        self.assertEqual(json_code["module"], "r.slope.aspect")
        self.assertEqual(len(json_code["inputs"]), 5)
        self.assertEqual(json_code["inputs"], inputs)
        self.assertEqual(json_code["outputs"], outputs)

    def test_v_out_ascii(self):
        args = ["v.out.ascii",
                "input=hospitals@PERMANENT",
                "output=myfile+TXT",
                "--json"]

        inputs = [
            {"param": "input", "value": "hospitals@PERMANENT"},
            {"param": "layer", "value": "1"},
            {"param": "type", "value": "point,line,boundary,centroid,area,face,kernel"},
            {"param": "format", "value": "point"},
            {"param": "separator", "value": "pipe"},
            {"param": "precision", "value": "8"}
        ]

        outputs = [
            {"export": {"format": "TXT", "type": "file"},
             "param": "output", "value": "$file::myfile"}
        ]

        stdout, stderr = subprocess.Popen(args, stdout=subprocess.PIPE).communicate()
        print(stdout)
        json_code = json.loads(decode(stdout))
        self.assertEqual(json_code["module"], "v.out.ascii")
        self.assertEqual(len(json_code["inputs"]), 6)
        self.assertEqual(json_code["inputs"], inputs)
        self.assertEqual(json_code["outputs"], outputs)

    def test_v_info(self):
        args = ["v.info",
                "map=hospitals@PERMANENT",
                "-c",
                "--json"]

        inputs = [
         {"param": "map", "value": "hospitals@PERMANENT"},
         {"param": "layer", "value": "1"}
       ]

        stdout, stderr = subprocess.Popen(args, stdout=subprocess.PIPE).communicate()
        print(stdout)
        json_code = json.loads(decode(stdout))
        self.assertEqual(json_code["module"], "v.info")
        self.assertEqual(len(json_code["inputs"]), 2)
        self.assertEqual(json_code["inputs"], inputs)


if __name__ == '__main__':
    from grass.gunittest.main import test

    test()
