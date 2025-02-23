"""
Name:       r.basins.fill
Purpose:    Tests r.basins.fill and its flags/options.

Author:     Sunveer Singh
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase


class TestRasterbasin(TestCase):
    celevation = "test_elevation"
    tgeology = "test_geology_30m"
    output = "basinsoutput"
    input = "test_lakes"
    rand_cell = "rand_cell"

    @classmethod
    def setUpClass(cls):
        """Copy raster maps."""
        cls.runModule("g.copy", raster=["elevation", cls.celevation], overwrite=True)
        cls.runModule("g.copy", raster=["geology_30m", cls.tgeology])
        cls.runModule("g.copy", raster=["lakes", cls.input])

        seed = 500
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.tgeology, flags="p")
        cls.runModule(
            "r.watershed",
            elevation="elevation",
            stream=cls.celevation,
            threshold="50000",
            overwrite=True,
        )
        cls.runModule(
            "r.geomorphon", elevation=cls.celevation, forms=cls.tgeology, overwrite=True
        )
        cls.runModule(
            "r.mapcalc",
            seed=seed,
            expression="rand_cell = rand(1, 200)",
            overwrite=True,
        )
        cls.runModule("r.thin", input=cls.input, output=cls.output)

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region and generated maps."""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[
                cls.output,
                cls.rand_cell,
                cls.tgeology,
                cls.celevation,
                cls.input,
            ],
        )
        cls.del_temp_region()

    def test_r_basins_fill_number_1(self):
        self.assertModule(
            "r.basins.fill",
            cnetwork=self.celevation,
            tnetwork=self.tgeology,
            output=self.output,
            number="1",
            overwrite=True,
        )
        self.assertRasterMinMax(
            map=self.input,
            refmin=34300,
            refmax=43600,
            msg="test_lakes in degrees must be between 34300 and 43600",
        )

    def test_r_basins_fill_number_3(self):
        self.assertModule(
            "r.basins.fill",
            cnetwork=self.celevation,
            tnetwork=self.tgeology,
            output=self.output,
            number="3",
            overwrite=True,
        )
        self.assertRasterMinMax(
            map="soilsID",
            refmin=18683,
            refmax=46555,
            msg="soilsID in degrees must be between 18683 and 46555",
        )

    def test_r_basins_fill_number_4(self):
        self.assertModule(
            "r.basins.fill",
            cnetwork=self.celevation,
            tnetwork=self.tgeology,
            output=self.output,
            number="4",
            overwrite=True,
        )
        self.assertRasterMinMax(
            map="landclass96",
            refmin=1,
            refmax=7,
            msg="landclass96 in degrees must be between 1 and 7",
        )

    def test_r_basins_fill_number_5(self):
        self.assertModule(
            "r.basins.fill",
            cnetwork=self.celevation,
            tnetwork=self.tgeology,
            output=self.output,
            number="5",
            overwrite=True,
        )
        self.assertRasterMinMax(
            map=self.rand_cell,
            refmin=1,
            refmax=199,
            msg="rand_cell in degrees must be between 1 and 199",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
