"""Test of v.out.ogr

@author Luís Moreira de Sousa
"""

from pathlib import Path
from grass.gunittest.case import TestCase


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

    area_map = "test_area_map"
    areas_string = """\
VERTI:
B  6
 11.6         29.55
 12.01        27.01
 15.25        26.68
 16.32        29.42
 13.49        31.06
 11.6         29.55
C  1 1
 13.85        28.69
 1     1
B  10
 18.49        34.1
 21.73        28.85
 28.24        33.28
 26.6         37.17
 23.9         34.92
 22.75        36.92
 18.86        37.17
 14.1         36.19
 14.19        32.54
 18.49        34.1
C  1 1
 21.11        34.03
 1     2
"""

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule(
            "v.in.ascii",
            input="-",
            output=cls.area_map,
            format="standard",
            stdin=cls.areas_string,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()
        cls.runModule("g.remove", type="vector", flags="f", name=cls.area_map)

    def tearDown(self):
        self.runModule(
            "g.remove", type="vector", flags="f", pattern=f"{self.temp_import}*"
        )
        for p in Path().glob(f"{self.test_map}*"):
            p.unlink()

    def test_gpkg_format(self):
        """Tests output to GeoPackage format"""

        self.assertModule(
            "v.out.ogr",
            "Export to GeoPackage Format",
            input=self.test_map,
            output=f"{self.test_map}.gpkg",
            format="GPKG",
        )

        # Import back to verify
        self.runModule(
            "v.in.ogr",
            input=f"{self.test_map}.gpkg",
            output=self.temp_import,
        )

        self.runModule("g.region", vector=self.temp_import)

        self.assertVectorFitsUnivar(
            map=self.temp_import,
            reference=self.univar_string,
            column=self.univar_col,
            precision=1e-8,
        )

    def test_shp_format(self):
        """Tests output to Shapefile format"""

        self.assertModule(
            "v.out.ogr",
            "Export to Shapefile Format",
            input=self.test_map,
            output=f"{self.test_map}.shp",
            format="ESRI_Shapefile",
        )

        # Import back to verify
        self.runModule(
            "v.in.ogr",
            input=f"{self.test_map}.shp",
            output=self.temp_import,
        )

        self.runModule("g.region", vector=self.temp_import)

        self.assertVectorFitsUnivar(
            map=self.temp_import,
            reference=self.univar_string,
            column=self.univar_col,
            precision=1e-8,
        )

    def test_geojson_format(self):
        """Tests output to GeoJSON format"""

        self.assertModule(
            "v.out.ogr",
            "Export to GeoJSON Format",
            input=self.test_map,
            output=f"{self.test_map}.json",
            format="GeoJSON",
        )
        self.assertFileExists(f"{self.test_map}.json")

    def test_geojson_format_no_attributes(self):
        """Tests output to GeoJSON format with no attributes"""
        self.assertModule(
            "v.out.ogr",
            "Export to GeoJSON Format",
            input=self.area_map,
            output=f"{self.test_map}.json",
            format="GeoJSON",
        )

        self.assertFileExists(f"{self.test_map}.json")


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
