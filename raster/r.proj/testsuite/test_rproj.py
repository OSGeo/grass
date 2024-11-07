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
dst_project = "nc_latlong"


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
        statics = """n=40930
        min=55.5787925720215
        max=156.038833618164
        mean=110.377538633405
        variance=412.751942806146"""

        self.run_rproj_test(method, statics)

    def test_bilinear(self):
        """Testing method bilinear"""
        # Set up variables and validation values
        method = "bilinear"
        statics = """n=40845
        min=56.3932914733887
        max=156.053298950195
        mean=110.389074372679
        variance=411.487781666933"""

        self.run_rproj_test(method, statics)

    def test_bicubic(self):
        """Testing method bicubic"""
        # Set up variables and validation values
        method = "bicubic"
        statics = """n=40677
        min=56.2407836914062
        max=156.061599731445
        mean=110.41701776258
        variance=411.382636894393"""

        self.run_rproj_test(method, statics)

    def test_lanczos(self):
        """Testing method lanczos"""
        # Set up variables and validation values
        method = "lanczos"
        statics = """n=40585
        min=56.2350921630859
        max=156.066345214844
        mean=110.421826400841
        variance=411.6875834341575"""

        self.run_rproj_test(method, statics)

    def test_bilinear_f(self):
        """Testing method bilinear_f"""
        # Set up variables and validation values
        method = "bilinear_f"
        statics = """n=40930
        min=55.5787925720215
        max=156.053298950195
        mean=110.376211041027
        variance=412.553041205029"""

        self.run_rproj_test(method, statics)

    def test_bicubic_f(self):
        """Testing method bicubic_f"""
        # Set up variables and validation values
        method = "bicubic_f"
        statics = """n=40930
        min=55.5787925720215
        max=156.061599731445
        mean=110.375897704515
        variance=412.693308000461"""

        self.run_rproj_test(method, statics)

    def test_lanczos_f(self):
        """Testing method lanczos_f"""
        # Set up variables and validation values
        method = "lanczos_f"
        statics = """n=40930
        min=55.5787925720215
        max=156.066345214844
        mean=110.375715222838
        variance=412.695433658258"""

        self.run_rproj_test(method, statics)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
