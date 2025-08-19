"""Test of v.out.ogr

@author Luís Moreira de Sousa
"""

from pathlib import Path
from shutil import rmtree
import grass.script as gs
from grass.gunittest.case import TestCase


class TestOgrExport(TestCase):
    # Vector map in NC test dataset
    test_map = "boundary_county"

    # Result of import tests
    temp_import = "test_ogr_import_map"

    # Temporary location
    temp_location = "temp_location"

    # Temporary Homolosine map
    temp_54052 = "temp_rand.gpkg"

    # Temporary environment file
    session_file = ""

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
        """Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):

        # Remove maps in temp mapset
        self.runModule(
            "g.remove", type="vector", flags="f", pattern=f"{self.temp_import}*"
        )
        # Remove temporary files
        for p in Path().glob(f"{self.test_map}*"):
            p.unlink()
        for p in Path().glob(f"{self.temp_54052}*"):
            p.unlink()
        if len(self.session_file) > 0:
            Path(self.session_file).unlink()

        # Remove temporary location
        env = gs.parse_command("g.gisenv")
        # Quotes and separator in environment variable string must be removed
        dbase = env["GISDBASE"].replace("'", "").replace(";", "")
        rmtree(f"{dbase}/{self.temp_location}", ignore_errors=True)

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

    def test_no_epsg_crs(self):
        """Tests output from a location/project lacking an EPSG code"""

        # Record original location to use corect GISDBASE
        env_orig = gs.parse_command("g.gisenv")

        # Create new location with CRS without EPSG code
        gs.run_command(
            "g.proj", wkt="data/ESRI54052.wkt", project=self.temp_location, flags="c"
        )

        # Create new session to avoid temporary region
        # Quotes and separator in environment variable string must be removed
        self.session_file, env_new = gs.core.create_environment(
            env_orig["GISDBASE"].replace("'", "").replace(";", ""),
            self.temp_location,
            "PERMANENT",
        )

        # Creates a random layer to test output
        gs.run_command("v.random", output="temp_rand", npoints=3, env=env_new)

        # Test output of a vector with non-EPSG CRS
        gs.run_command(
            "v.out.ogr",
            input="temp_rand",
            output=self.temp_54052,
            format="GPKG",
            dsco="a_srs=ESRI:54052",
            env=env_new,
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
