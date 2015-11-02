"""Test of r.in.gdal

@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase

class TestGdalImport(TestCase):

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region
        """
        cls.del_temp_region()

    def tearDown(self):
        self.runModule("g.remove", type="raster", flags="f",
                       pattern="test_gdal_import_map*")


    def test_1(self):

        self.assertModule("r.in.gdal", "Import GTiff Format",
                          input="data/elevation.tif",
                          output="test_gdal_import_map")

        self.runModule("g.region", raster="test_gdal_import_map")

        # Output of r.univar
        univar_string="""n=20250
                         null_cells=0
                         cells=20250
                         min=56.1364936828613
                         max=156.221710205078
                         range=100.085216522217
                         mean=110.358078733845
                         mean_of_abs=110.358078733845
                         stddev=20.3247267738233
                         variance=413.09451843057
                         coeff_var=18.4170719597623
                         sum=2234751.09436035"""

        self.assertRasterFitsUnivar(raster="test_gdal_import_map",  reference=univar_string,
                                    precision=3)

    def test_2(self):

        self.assertModule("r.in.gdal", "Import GTiff Format",
                          input="data/elevation.tiff",
                          output="test_gdal_import_map")

        self.runModule("g.region", raster="test_gdal_import_map")

        # Output of r.univar
        univar_string="""n=20250
                         null_cells=0
                         cells=20250
                         min=56.1364936828613
                         max=156.221710205078
                         range=100.085216522217
                         mean=110.358078733845
                         mean_of_abs=110.358078733845
                         stddev=20.3247267738233
                         variance=413.09451843057
                         coeff_var=18.4170719597623
                         sum=2234751.09436035"""

        self.assertRasterFitsUnivar(raster="test_gdal_import_map",  reference=univar_string,
                                    precision=3)

    def test_3(self):

        self.assertModule("r.in.gdal", "Import AAIGrid Format",
                          input="data/elevation.asc",
                          output="test_gdal_import_map")

        self.runModule("g.region", raster="test_gdal_import_map")

        # Output of r.univar
        univar_string="""n=20250
                         null_cells=0
                         cells=20250
                         min=56.1364936828613
                         max=156.221710205078
                         range=100.085216522217
                         mean=110.358078733845
                         mean_of_abs=110.358078733845
                         stddev=20.3247267738233
                         variance=413.09451843057
                         coeff_var=18.4170719597623
                         sum=2234751.09436035"""

        self.assertRasterFitsUnivar(raster="test_gdal_import_map",  reference=univar_string,
                                    precision=3)

    def test_4(self):

        self.assertModule("r.in.gdal", "Import netCDF Format",
                          input="data/elevation.nc",
                          output="test_gdal_import_map")

        self.runModule("g.region", raster="test_gdal_import_map")

        # Output of r.univar
        univar_string="""n=20250
                         null_cells=0
                         cells=20250
                         min=56.1364936828613
                         max=156.221710205078
                         range=100.085216522217
                         mean=110.358078733845
                         mean_of_abs=110.358078733845
                         stddev=20.3247267738233
                         variance=413.09451843057
                         coeff_var=18.4170719597623
                         sum=2234751.09436035"""

        self.assertRasterFitsUnivar(raster="test_gdal_import_map",  reference=univar_string,
                                    precision=3)

    def test_netCDF_3d_1(self):

        self.assertModule("r.in.gdal", "Import netCDF Format",
                          input="data/elevation3d.nc",
                          num_digits="3",
                          flags="o",
                          output="test_gdal_import_map")

        # Output of r.info
        info_string="""north=228500
                       south=215000
                       east=644640
                       west=629640
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""

        self.assertRasterFitsInfo(raster="test_gdal_import_map.001",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.002",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.003",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.004",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.005",  reference=info_string,
                                    precision=3)

    def test_netCDF_3d_2(self):

        self.assertModule("r.in.gdal", "Import netCDF Format",
                          input="data/elevation3d.nc",
                          num_digits=0,
                          offset=100,
                          flags="o",
                          output="test_gdal_import_map")

        # Output of r.info
        info_string="""north=228500
                       south=215000
                       east=644640
                       west=629640
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""

        self.assertRasterFitsInfo(raster="test_gdal_import_map.101",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.102",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.103",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.104",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.105",  reference=info_string,
                                    precision=3)

    def test_netCDF_3d_3(self):

        self.assertModule("r.in.gdal", "Import netCDF Format",
                          input="data/elevation3d.nc",
                          num_digits=5,
                          offset=100,
                          flags="o",
                          output="test_gdal_import_map")

        # Output of r.info
        info_string="""north=228500
                       south=215000
                       east=644640
                       west=629640
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""

        self.assertRasterFitsInfo(raster="test_gdal_import_map.00101",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.00102",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.00103",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.00104",  reference=info_string,
                                    precision=3)
        self.assertRasterFitsInfo(raster="test_gdal_import_map.00105",  reference=info_string,
                                    precision=3)

    def test_netCDF_3d_4(self):

        self.assertModule("r.in.gdal", "Import netCDF Format",
                          input="data/elevation3d.nc",
                          num_digits="3",
                          flags="o",
                          band=2,
                          output="test_gdal_import_map")

        # Output of r.info
        info_string="""north=228500
                       south=215000
                       east=644640
                       west=629640
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""

        self.assertRasterFitsInfo(raster="test_gdal_import_map",  reference=info_string,
                                    precision=3)

    def test_netCDF_3d_5(self):
        """Test the output map names file option"""

        self.assertModule("r.in.gdal", "Import netCDF Format",
                          input="data/elevation3d.nc",
                          num_digits=10,
                          offset=100,
                          flags="o",
                          map_names_file="map_names_file.txt",
                          output="test_gdal_import_map")

        map_list="""test_gdal_import_map.0000000101
test_gdal_import_map.0000000102
test_gdal_import_map.0000000103
test_gdal_import_map.0000000104
test_gdal_import_map.0000000105
"""

        text_from_file = open("map_names_file.txt", "r").read()

        self.assertLooksLike(map_list, text_from_file)


class TestGdalImportFails(TestCase):

    def test_error_handling_1(self):
        # Wrong number of digits
        self.assertModuleFail("r.in.gdal",
                              input="data/elevation.nc",
                              num_digits="-1",
                              output="test_gdal_import_map")

    def test_error_handling_2(self):
        # No location specified
        self.assertModuleFail("r.in.gdal",
                              input="data/elevation.nc",
                              flags="c",
                              output="test_gdal_import_map")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()


