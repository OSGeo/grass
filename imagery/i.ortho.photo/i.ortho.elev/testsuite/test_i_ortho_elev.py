import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIOrthoElev(TestCase):
    """Regression testsuite for i.ortho.elev module"""

    test_group = "test_ortho_elev_group"
    tmp_raster = "temp_raster"
    elev_map = "elevation_map"
    scaled_elev_map = "scaled_elevation_map"
    tmp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up input region and rasters for test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.tmp_raster} = row()", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", expression=f"{cls.elev_map} = 100 + row()", overwrite=True
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.scaled_elev_map} = int(row()/5)",
            overwrite=True,
        )
        cls.tmp_rasters.extend([cls.tmp_raster, cls.elev_map, cls.scaled_elev_map])

        cls.runModule(
            "i.group",
            group=cls.test_group,
            input=f"{cls.tmp_raster},{cls.elev_map},{cls.scaled_elev_map}",
        )

    def setUp(self):
        """Set the target location and mapset for the test imagery group."""
        self.runModule(
            "i.target",
            group=self.test_group,
            location=gs.gisenv()["LOCATION_NAME"],
            mapset=gs.gisenv()["MAPSET"],
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary raster maps and imagery group, and reset region."""
        gs.run_command("g.remove", flags="f", type="raster", name=cls.tmp_rasters)
        gs.run_command("g.remove", flags="f", type="group", name=cls.test_group)
        cls.del_temp_region()

    def test_set_basic_elevation(self):
        """Assigns an elevation map to the ortho group and verifies metadata output"""
        self.assertModule(
            "i.ortho.elev",
            group=self.test_group,
            elevation=self.elev_map,
        )
        output = gs.read_command("i.ortho.elev", group=self.test_group, flags="p")
        self.assertIn(f"map:\t\t\t{self.elev_map}", output)

    def test_all_metadata(self):
        """Assigns an elevation map with math expression, unit, and null value and checks all metadata fields"""
        self.assertModule(
            "i.ortho.elev",
            group=self.test_group,
            elevation=self.scaled_elev_map,
            math_expression="x*10+50",
            units="meters",
            null_value="-1",
        )

        output = gs.read_command("i.ortho.elev", group=self.test_group, flags="p")

        self.assertIn(f"map:\t\t\t{self.scaled_elev_map}", output)
        self.assertIn("math expression:\tx*10+50", output)
        self.assertIn("units:\t\t\tmeters", output)
        self.assertIn("nodata value:\t\t-1", output)

    def test_parameter_persistence(self):
        """Ensure previously set parameters persist after updating the elevation map"""
        self.runModule(
            "i.ortho.elev",
            group=self.test_group,
            elevation=self.elev_map,
            units="meters",
            null_value="-1",
        )
        initial_output = gs.read_command(
            "i.ortho.elev", group=self.test_group, flags="p"
        )
        self.assertIn(f"map:\t\t\t{self.elev_map}", initial_output)
        self.assertIn("units:\t\t\tmeters", initial_output)

        self.runModule(
            "i.ortho.elev",
            group=self.test_group,
            elevation=self.scaled_elev_map,
            units="meters",
            null_value="-1",
        )
        updated_output = gs.read_command(
            "i.ortho.elev", group=self.test_group, flags="p"
        )
        self.assertIn(f"map:\t\t\t{self.scaled_elev_map}", updated_output)
        self.assertIn("units:\t\t\tmeters", updated_output)


if __name__ == "__main__":
    test()
