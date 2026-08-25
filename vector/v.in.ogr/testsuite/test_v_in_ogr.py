"""Test of v.in.ogr

@author Markus Neteler
"""

from grass.gunittest.case import TestCase


class TestOgrImport(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        self.runModule(
            "g.remove", type="vector", flags="f", pattern="test_ogr_import_map*"
        )

    def test_1(self):
        self.assertModule(
            "v.in.ogr",
            "Import GeoPackage Format",
            input="data/firestations.gpkg",
            output="test_ogr_import_map",
        )

        self.runModule("g.region", vector="test_ogr_import_map")

        # Output of v.univar
        univar_string = """n=71
nmissing=0
nnull=0
min=0
max=4
range=4
sum=21
mean=0.295775
mean_abs=0.295775
population_stddev=0.719827
population_variance=0.518151
population_coeff_variation=2.4337
sample_stddev=0.724951
sample_variance=0.525553
kurtosis=8.86174
skewness=2.82871"""

        self.assertVectorFitsUnivar(
            map="test_ogr_import_map",
            reference=univar_string,
            column="WATER_RESC",
            precision=1e-8,
        )

    def test_2(self):
        self.assertModule(
            "v.in.ogr",
            "Import 3D SHAPE Format",
            input="data/precip_30ynormals_3d.shp",
            output="test_ogr_import_map",
        )

        self.runModule("g.region", vector="test_ogr_import_map")

        # Output of v.info
        univar_string = """n=136
nmissing=0
nnull=0
min=2.4384
max=1615.44
range=1613
sum=40827.7
mean=300.203
mean_abs=300.203
population_stddev=320.783
population_variance=102902
population_coeff_variation=1.06855
sample_stddev=321.969
sample_variance=103664
kurtosis=1.98052
skewness=1.45954"""

        self.assertVectorFitsUnivar(
            map="test_ogr_import_map",
            reference=univar_string,
            column="elev",
            precision=1e-8,
        )


# class TestOgrImportFails(TestCase):
#    def test_error_handling_1(self):
#        # Wrong type
#        self.assertModuleFail(
#            "v.in.ogr",
#            input="data/firestations.gpkg",
#            type="kernel",
#            output="test_ogr_import_map",
#        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
