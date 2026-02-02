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
import os
from grass.script import core as gcore
import tempfile
import pathlib

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

    def test_profile_json(self):
        # ensure region is set to the test elevation
        self.runModule("g.region", raster=self.inele)
        region = gcore.region()
        east = float(region.get("east", region["e"]))
        west = float(region.get("west", region["w"]))
        north = float(region.get("north", region["n"]))
        south = float(region.get("south", region["s"]))
        e = (east + west) / 2.0
        n = (north + south) / 2.0
        fd, tmp = tempfile.mkstemp(prefix="rgeom_profile_", suffix=".json")
        os.close(fd)
        try:
            self.runModule(
                "r.geomorphon",
                elevation=self.inele,
                search=3,
                profiledata=tmp,
                profileformat="json",
                coordinates=(e, n),
            )
            self.assertTrue(pathlib.Path(tmp).exists())
            self.assertGreater(pathlib.Path(tmp).stat().st_size, 0)
            with open(tmp, encoding="utf-8") as fh:
                data = json.load(fh)
            # basic sanity checks on JSON structure
            self.assertIn("final_results", data)
            self.assertIn("computation_parameters", data)
            self.assertIsInstance(data["final_results"], dict)
            self.assertIsInstance(data["computation_parameters"], dict)
        finally:
            if pathlib.Path(tmp).exists():
                os.remove(tmp)


if __name__ == "__main__":
    test()
