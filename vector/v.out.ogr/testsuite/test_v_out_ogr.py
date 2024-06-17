"""Test of v.in.ogr

@author Luís Moreira de Sousa
"""

from grass.gunittest.case import TestCase
from pathlib import Path


class TestOgrExport(TestCase):

    # Vector map in NC test dataset
    test_map = "boundary_county"

    # Result of import tests
    temp_import = "test_ogr_import_map"

    # Column on which to test v.univar
    univar_col = "PERIMETER"

    # Output of v.univar
    univar_string = """n=926
nmissing=0
nnull=0
min=9.64452
max=3.70609e+06
range=3.70608e+06
sum=1.1223e+08
mean=121199
mean_abs=121199
population_stddev=342855
population_variance=1.17549e+11
population_coeff_variation=2.82886
sample_stddev=343040
sample_variance=1.17676e+11
kurtosis=33.681
skewness=4.86561
"""

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
            "g.remove", type="vector", flags="f", pattern="%s*" % self.temp_import
        )
        for p in Path(".").glob("%s*" % self.test_map):
            p.unlink()

    def test_1(self):
        self.assertModule(
            "v.out.ogr",
            "Export to GeoPackage Format",
            input=self.test_map,
            output="%s.gpkg" % self.test_map,
            format="GPKG",
        )

        # Import back to verify
        self.runModule(
            "v.in.ogr",
            input="%s.gpkg" % self.test_map,
            output=self.temp_import,
        )

        self.runModule("g.region", vector=self.temp_import)

        self.assertVectorFitsUnivar(
            map=self.temp_import,
            reference=self.univar_string,
            column=self.univar_col,
            precision=1e-8,
        )

    def test_2(self):
        self.assertModule(
            "v.out.ogr",
            "Export to Shapefile Format",
            input=self.test_map,
            output="%s.shp" % self.test_map,
            format="ESRI_Shapefile",
        )

        # Import back to verify
        self.runModule(
            "v.in.ogr",
            input="%s.shp" % self.test_map,
            output=self.temp_import,
        )

        self.runModule("g.region", vector=self.temp_import)

        self.assertVectorFitsUnivar(
            map=self.temp_import,
            reference=self.univar_string,
            column=self.univar_col,
            precision=1e-8,
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
