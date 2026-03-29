"""Test of r.external

@author Markus Neteler
"""

from grass.gunittest.case import TestCase


class TestGdalImport(TestCase):
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
            "g.remove", type="raster", flags="f", pattern="test_external_map*"
        )

    def test_1(self):
        self.assertModule(
            "r.external",
            "Register GTiff Format",
            input="data/elevation.tif",
            output="test_external_map",
        )

        self.runModule("g.region", raster="test_external_map")

        # Output of r.univar
        univar_string = """n=20250
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

        self.assertRasterFitsUnivar(
            raster="test_external_map", reference=univar_string, precision=3
        )

    def test_2(self):
        self.assertModule(
            "r.external",
            "Register GTiff Format",
            input="data/elevation.tiff",
            output="test_external_map",
        )

        self.runModule("g.region", raster="test_external_map")

        # Output of r.univar
        univar_string = """n=20250
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

        self.assertRasterFitsUnivar(
            raster="test_external_map", reference=univar_string, precision=3
        )

    def test_3(self):
        self.assertModule(
            "r.external",
            "Register AAIGrid Format",
            input="data/elevation.asc",
            output="test_external_map",
        )

        self.runModule("g.region", raster="test_external_map")

        # Output of r.univar
        univar_string = """n=20250
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

        self.assertRasterFitsUnivar(
            raster="test_external_map", reference=univar_string, precision=3
        )

    def test_4(self):
        self.assertModule(
            "r.external",
            "Register netCDF Format",
            input="data/elevation.nc",
            output="test_external_map",
        )

        self.runModule("g.region", raster="test_external_map")

        # Output of r.univar
        univar_string = """n=20250
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

        self.assertRasterFitsUnivar(
            raster="test_external_map", reference=univar_string, precision=3
        )

    def test_link_raster_simple(self):
        self.assertModule(
            "r.external",
            input="data/elevation.asc",
            output="test_external_map",
            flags="e",
        )

        self.runModule("g.region", raster="test_external_map")

    def test_link_raster_with_title(self):
        self.assertModule(
            "r.external",
            input="data/elevation.asc",
            output="test_external_map",
            title="Test Elevation Map",
            flags="e",
        )

    def test_link_raster_overwrite(self):
        self.runModule(
            "r.external",
            input="data/elevation.asc",
            output="test_external_map",
            flags="e",
        )

        self.assertModule(
            "r.external",
            input="data/elevation.asc",
            output="test_external_map",
            flags="e",
            overwrite=True,
        )

    def test_invalid_input_fails(self):
        self.assertModuleFail(
            "r.external",
            input="non_existent_file.tif",
            output="should_not_exist",
            flags="e",
        )

    def test_link_multiple_rasters(self):
        self.runModule("g.remove", type="raster", name="map_1", flags="f")
        self.runModule("g.remove", type="raster", name="map_2", flags="f")

        self.assertModule(
            "r.external", input="data/elevation.asc", output="map_1", flags="e"
        )

        self.assertModule(
            "r.external", input="data/elevation.asc", output="map_2", flags="e"
        )

    def test_link_sequential_calls(self):
        self.runModule("g.remove", type="raster", name="first_map", flags="f")
        self.runModule("g.remove", type="raster", name="second_map", flags="f")

        self.runModule(
            "r.external", input="data/elevation.asc", output="first_map", flags="e"
        )

        self.assertModule(
            "r.external", input="data/elevation.asc", output="second_map", flags="e"
        )

    def test_link_with_project_context(self):
        self.assertModule(
            "r.external",
            input="data/elevation.asc",
            output="test_external_map",
            flags="e",
        )

        self.runModule("g.list", type="raster", pattern="test_external_map")

    def test_netCDF_3d_1(self):
        self.assertModule(
            "r.external",
            "Register netCDF Format",
            input="data/elevation3d.nc",
            flags="o",
            output="test_external_map",
        )

        # Output of r.info
        info_string = """north=228500
                       south=215000
                       east=645000
                       west=630000
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""

        self.assertRasterFitsInfo(
            raster="test_external_map.1", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.2", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.3", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.4", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.5", reference=info_string, precision=3
        )

    def test_netCDF_3d_2(self):
        self.assertModule(
            "r.external",
            "Register netCDF Format",
            input="data/elevation3d.nc",
            flags="o",
            output="test_external_map",
        )

        # Output of r.info
        info_string = """north=228500
                       south=215000
                       east=645000
                       west=630000
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""

        self.assertRasterFitsInfo(
            raster="test_external_map.1", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.2", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.3", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.4", reference=info_string, precision=3
        )
        self.assertRasterFitsInfo(
            raster="test_external_map.5", reference=info_string, precision=3
        )

    def test_netCDF_3d_3(self):
        self.assertModule(
            "r.external",
            "Register netCDF Format",
            input="data/elevation3d.nc",
            flags="o",
            band=2,
            output="test_external_map",
        )

        # Output of r.info
        info_string = """north=228500
                       south=215000
                       east=645000
                       west=630000
                       nsres=100
                       ewres=100
                       rows=135
                       cols=150
                       cells=20250
                       datatype=FCELL
                       ncats=0"""

        self.assertRasterFitsInfo(
            raster="test_external_map", reference=info_string, precision=3
        )


class TestExternalImportFails(TestCase):
    def test_error_handling_1(self):
        # Wrong band number
        self.assertModuleFail(
            "r.external",
            input="data/elevation.nc",
            band="-1",
            output="test_external_map",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
