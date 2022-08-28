"""Test of r.univar

@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase


class TestRasterUnivar(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        self.runModule("g.remove", flags="f", type="raster", name="map_a")
        self.runModule("g.remove", flags="f", type="raster", name="map_b")
        self.runModule("g.remove", flags="f", type="raster", name="zone_map")

    def setUp(self):
        """Create input data"""
        self.runModule("g.region", res=1, n=90, s=0, w=0, e=90)
        self.runModule(
            "r.mapcalc", expression="map_a = 100 + row() + col()", overwrite=True
        )
        self.runModule(
            "r.mapcalc", expression="map_b = 200 + row() + col()", overwrite=True
        )
        self.runModule(
            "r.mapcalc", expression="zone_map = if(row() < 20, 1,2)", overwrite=True
        )

    def test_1(self):
        # Output of r.univar
        univar_string = """n=8100
        null_cells=0
        cells=8100
        min=102
        max=280
        range=178
        mean=191
        mean_of_abs=191
        sum=1547100"""

        self.assertRasterFitsUnivar(
            raster="map_a", reference=univar_string, precision=6
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_2(self):
        # Output of r.univar
        univar_string = """n=81
        null_cells=0
        cells=81
        min=112
        max=272
        range=160
        mean=192
        mean_of_abs=192
        sum=15552"""

        self.runModule("g.region", res=10)
        self.assertRasterFitsUnivar(
            raster="map_a", reference=univar_string, precision=6
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_3(self):
        """
        Check the -r flag
        :return:
        """

        univar_string = """n=8100
        null_cells=0
        cells=8100
        min=102
        max=280
        range=178
        mean=191
        mean_of_abs=191
        sum=1547100"""

        self.runModule("g.region", res=10)
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="rg",
            reference=univar_string,
            precision=6,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map="map_a",
            flags="rg",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_multiple_1(self):
        # Output of r.univar
        univar_string = """n=16200
        null_cells=0
        cells=16200
        min=102
        max=380
        range=278
        mean=241
        mean_of_abs=241
        sum=3904200"""

        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            reference=univar_string,
            precision=6,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_multiple_2(self):
        # Output of r.univar
        univar_string = """n=162
        null_cells=0
        cells=162
        min=112
        max=372
        range=260
        mean=241
        mean_of_abs=241
        sum=39204"""

        self.runModule("g.region", res=10)
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="g",
            reference=univar_string,
            precision=6,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_multiple_3(self):
        """
        Check the -r flag
        :return:
        """

        # Output of r.univar
        univar_string = """n=16200
        null_cells=0
        cells=16200
        min=102
        max=380
        range=278
        mean=241
        mean_of_abs=241
        sum=3904200"""

        self.runModule("g.region", res=10)
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            reference=univar_string,
            precision=6,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            flags="rg",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_1_zone(self):
        """
        one map and zone
        :return:
        """

        # Output of r.univar
        univar_string = """zone=1;
                        n=1710
                        null_cells=0
                        cells=1710
                        min=102
                        max=209
                        range=107
                        mean=155.5
                        mean_of_abs=155.5
                        sum=265905
                        zone=2;
                        n=6390
                        null_cells=0
                        cells=1710
                        min=121
                        max=280
                        range=159
                        mean=200.5
                        mean_of_abs=200.5
                        sum=1281195"""

        self.runModule("g.region", res=1)
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a"],
            zones="zone_map",
            flags="g",
            reference=univar_string,
            precision=6,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a"],
            zones="zone_map",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_2_zone(self):
        """
        multiple maps and zone
        :return:
        """

        # Output of r.univar
        univar_string = """zone=1;
                        n=3420
                        null_cells=0
                        cells=3420
                        min=102
                        max=309
                        range=207
                        mean=205.5
                        mean_of_abs=205.5
                        stddev=56.6119834192962
                        variance=3204.91666666667
                        coeff_var=27.5484104230152
                        sum=702810
                        zone=2;
                        n=12780
                        null_cells=0
                        cells=3420
                        min=121
                        max=380
                        range=259
                        mean=250.5
                        mean_of_abs=250.5
                        stddev=59.9576239244574
                        variance=3594.91666666667
                        coeff_var=23.9351792113602
                        sum=3201390"""

        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="g",
            reference=univar_string,
            precision=6,
            sep="=",
        )
        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="g",
            nprocs=4,
            reference=univar_string,
            precision=6,
            sep="=",
        )

    def test_3_zone(self):
        """
        multiple maps and zone
        :return:
        """

        # Output of r.univar
        univar_string = """zone=1;
                        n=3420
                        null_cells=0
                        cells=3420
                        min=102
                        max=309
                        range=207
                        mean=205.5
                        mean_of_abs=205.5
                        stddev=56.6119834192962
                        variance=3204.91666666667
                        coeff_var=27.5484104230152
                        sum=702810
                        first_quartile=155
                        median=205.5
                        third_quartile=255
                        percentile_90=282
                        zone=2;
                        n=12780
                        null_cells=0
                        cells=3420
                        min=121
                        max=380
                        range=259
                        mean=250.5
                        mean_of_abs=250.5
                        stddev=59.9576239244574
                        variance=3594.91666666667
                        coeff_var=23.9351792113602
                        sum=3201390
                        first_quartile=200
                        median=250.5
                        third_quartile=300
                        percentile_90=330"""

        self.assertModuleKeyValue(
            module="r.univar",
            map=["map_a", "map_b"],
            zones="zone_map",
            flags="ge",
            reference=univar_string,
            precision=6,
            sep="=",
        )


class TestAccumulateFails(TestCase):
    def test_error_handling(self):
        # No vector map, no strds, no coordinates
        self.assertModuleFail("r.univar", flags="r", map="map_a", zones="map_b")


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
