"""
Name:       r.texture test
Purpose:    Tests r.texture and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
            Chung-Yuan Liang, modified in 2024
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.checkers import text_to_keyvalue
from grass.gunittest.gmodules import call_module
import os

raster_info = """north=35.8096296297222
south=35.6874074075
east=-78.608
west=-78.7746666666667
nsres=0.000740740740740727
ewres=0.000666666666666686
rows=165
cols=250
cells=41250"""

src_project = "nc_spm_full_v2alpha2"


class TestRasterreport(TestCase):
    input = "elevation"

    @classmethod
    def setUpClass(cls):
        cls.runModule("g.proj", project="nc_latlong", epsg="4326", flags="c")
        cls.runModule("g.mapset", mapset="PERMANENT", project="nc_latlong")

    @classmethod
    def tearDownClass(cls):
        dbase = call_module("g.gisenv", get="GISDBASE")
        os.system(f"rm -rf {dbase}/nc_latlong")

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
        ## Get the boundary and set up region for the projected map
        stdout = call_module(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            input=self.input,
            method=method,
            flags="g",
        )
        settings = dict([line.split("=") for line in stdout.split()])

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

        ## Project the map
        self.assertModule(
            "r.proj",
            project=src_project,
            mapset="PERMANENT",
            input=self.input,
            output=output,
            method=method,
            quiet=True,
        )

        ## Validate the output
        self.assertRasterFitsUnivar(output, reference=statics, precision=1e-2)
        self.assertRasterFitsInfo(output, reference=raster_info, precision=1e-2)

    def test_nearest(self):
        """Testing method nearest"""
        ## Set up variables and validation values
        method = "nearest"
        statics = """n=40929
        min=55.5787925720215
        max=156.038833618164
        mean=110.377588991481
        variance=412.79107041757"""

        self.run_rproj_test(method, statics)

    def test_bilinear(self):
        """Testing method bilinear"""
        ## Set up variables and validation values
        method = "bilinear"
        statics = """n=40846
        min=56.4586868286133
        max=156.053405761719
        mean=110.388441504306
        variance=411.490915468646"""

        self.run_rproj_test(method, statics)

    def test_bicubic(self):
        """Testing method bicubic"""
        ## Set up variables and validation values
        method = "bicubic"
        statics = """n=40678
        min=56.2927856445312
        max=156.06169128418
        mean=110.417365826772
        variance=411.302683091198"""

        self.run_rproj_test(method, statics)

    def test_lanczos(self):
        """Testing method lanczos"""
        ## Set up variables and validation values
        method = "lanczos"
        statics = """n=40587
        min=56.2883224487305
        max=156.066925048828
        mean=110.423209871101
        variance=411.631827344207"""

        self.run_rproj_test(method, statics)

    def test_bilinear_f(self):
        """Testing method bilinear_f"""
        ## Set up variables and validation values
        method = "bilinear_f"
        statics = """n=40929
        min=55.5787925720215
        max=156.053405761719
        mean=110.376596053808
        variance=412.568369942694"""

        self.run_rproj_test(method, statics)

    def test_bicubic_f(self):
        """Testing method bicubic_f"""
        ## Set up variables and validation values
        method = "bicubic_f"
        statics = """n=40929
        min=55.5787925720215
        max=156.06169128418
        mean=110.37642902228
        variance=412.706934718806"""

        self.run_rproj_test(method, statics)

    def test_lanczos_f(self):
        """Testing method lanczos_f"""
        ## Set up variables and validation values
        method = "lanczos_f"
        statics = """n=40929
        min=55.5787925720215
        max=156.066925048828
        mean=110.376264598194
        variance=412.710534286582"""

        self.run_rproj_test(method, statics)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
