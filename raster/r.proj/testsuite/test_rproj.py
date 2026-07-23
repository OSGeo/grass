#!/usr/bin/env python

##############################################################################
# MODULE:    r.proj
#
# AUTHOR(S): Chung-Yuan Liang <cyliang368 AT gmail.com>
#
# PURPOSE:   Unit tests for r.proj
#
# COPYRIGHT: (C) 2024 Chung-Yuan Liang and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import call_module
import shutil
import json

raster_info = """north=35.8096296297222
south=35.6874074075
east=-78.608
west=-78.7746666666667
nsres=0.000740740740740727
ewres=0.000666666666666686
rows=165
cols=250
cells=41250"""

src_project = "nc_spm_full_v2beta1"
dst_project = "nc_latlong"

raster_maps = [
    "landclass96",
    "lsat7_2002_40",
    "elevation",
    "lsat7_2002_70",
    "boundary_county_500m",
    "basin",
]


class TestRasterreport(TestCase):
    input = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.runModule("g.proj", project=dst_project, epsg="4326", flags="c")
        cls.runModule("g.mapset", mapset="PERMANENT", project=dst_project)

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.mapset", mapset="PERMANENT", project=src_project)
        dbase = call_module("g.gisenv", get="GISDBASE")
        shutil.rmtree(f"{dbase}/{dst_project}")

    def run_rproj_test(self, method, statics):
        """The main function to run r.proj check rsults according to the method

        Parameters
        ----------
        method : str
            The method to be used for r.proj
        statics : str
            The expected statics of the output raster
        """
        output = method
        # Get the boundary and set up region for the projected map
        flag_output = call_module(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            input=self.input,
            method=method,
            flags="g",
        )
        settings = dict([line.split("=") for line in flag_output.split()])

        call_module(
            "g.region",
            n=settings["n"],
            s=settings["s"],
            e=settings["e"],
            w=settings["w"],
            rows=settings["rows"],
            cols=settings["cols"],
            flags="a",
            res=1,
        )

        option_output = call_module(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            input=self.input,
            method=method,
            flags="p",
            format="shell",
        )

        self.assertEqual(flag_output, option_output)

        # Project the map
        self.assertModule(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            input=self.input,
            output=output,
            method=method,
            quiet=True,
        )

        # Validate the output
        self.assertRasterFitsUnivar(output, reference=statics, precision=1e-7)
        self.assertRasterFitsInfo(output, reference=raster_info, precision=1e-7)

    def test_nearest(self):
        """Testing method nearest"""
        # Set up variables and validation values
        method = "nearest"
        statics = """n=40929
        min=55.5787925720215
        max=156.038833618164
        mean=110.377588991481
        variance=412.791070416939"""

        self.run_rproj_test(method, statics)

    def test_bilinear(self):
        """Testing method bilinear"""
        # Set up variables and validation values
        method = "bilinear"
        statics = """n=40846
        min=56.4586868286133
        max=156.053405761719
        mean=110.388441504306
        variance=411.490915467985"""

        self.run_rproj_test(method, statics)

    def test_bicubic(self):
        """Testing method bicubic"""
        # Set up variables and validation values
        method = "bicubic"
        statics = """n=40678
        min=56.2927856445312
        max=156.06169128418
        mean=110.417365826772
        variance=411.302683090515"""

        self.run_rproj_test(method, statics)

    def test_lanczos(self):
        """Testing method lanczos"""
        # Set up variables and validation values
        method = "lanczos"
        statics = """n=40587
        min=56.2883224487305
        max=156.066925048828
        mean=110.423209871101
        variance=411.631827343473"""

        self.run_rproj_test(method, statics)

    def test_bilinear_f(self):
        """Testing method bilinear_f"""
        # Set up variables and validation values
        method = "bilinear_f"
        statics = """n=40929
        min=55.5787925720215
        max=156.053405761719
        mean=110.376596053808
        variance=412.568369942043"""

        self.run_rproj_test(method, statics)

    def test_bicubic_f(self):
        """Testing method bicubic_f"""
        # Set up variables and validation values
        method = "bicubic_f"
        statics = """n=40929
        min=55.5787925720215
        max=156.06169128418
        mean=110.37642902228
        variance=412.70693471812"""

        self.run_rproj_test(method, statics)

    def test_lanczos_f(self):
        """Testing method lanczos_f"""
        # Set up variables and validation values
        method = "lanczos_f"
        statics = """n=40929
        min=55.5787925720215
        max=156.066925048828
        mean=110.376264598194
        variance=412.710534285851"""

        self.run_rproj_test(method, statics)

    def test_list_output_plain(self):
        """Test plain output of available raster maps in input mapset ."""
        result = call_module(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            flags="l",
        )
        result_list = result.split()

        for r_map in raster_maps:
            self.assertIn(
                r_map, result_list, f"'{r_map}' not found in raster map list (plain)"
            )

    def test_list_output_json(self):
        """Test JSON output of available raster maps in input mapset."""
        output = call_module(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            flags="l",
            format="json",
        )
        result = json.loads(output)

        for r_map in raster_maps:
            self.assertIn(
                r_map, result, f"'{r_map}' not found in raster map list (JSON)"
            )

    def test_print_output_plain(self):
        """Test printing input map bounds in the current projection (plain format)."""
        result = call_module(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            input=self.input,
            flags="p",
        ).splitlines()

        expected = [
            "Source cols: 1500",
            "Source rows: 1350",
            "Local north: 35:48:34.619215N",
            "Local south: 35:41:15.051632N",
            "Local west: 78:46:28.642843W",
            "Local east: 78:36:29.900338W",
        ]

        self.assertListEqual(result, expected, "Mismatch in print output (plain)")

    def test_print_output_json(self):
        """Test printing input map bounds in the current projection (JSON format)."""
        output = call_module(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            input=self.input,
            flags="p",
            format="json",
        )
        result = json.loads(output)

        expected = {
            "cols": 1500,
            "east": -78.60830564948812,
            "north": 35.80961644859374,
            "rows": 1350,
            "south": 35.68751434209047,
            "west": -78.77462301207872,
        }

        self.assertEqual(
            result["cols"], expected["cols"], msg="Mismatch in print output (JSON)"
        )
        self.assertAlmostEqual(
            result["east"], expected["east"], msg="Mismatch in print output (JSON)"
        )
        self.assertAlmostEqual(
            result["north"], expected["north"], msg="Mismatch in print output (JSON)"
        )
        self.assertEqual(
            result["rows"], expected["rows"], msg="Mismatch in print output (JSON)"
        )
        self.assertAlmostEqual(
            result["south"], expected["south"], msg="Mismatch in print output (JSON)"
        )
        self.assertAlmostEqual(
            result["west"], expected["west"], msg="Mismatch in print output (JSON)"
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
