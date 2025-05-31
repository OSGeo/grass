import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIGroup(TestCase):
    """Regression tests for i.group module."""

    test_group = "test_group"
    test_subgroup1 = "test_subgroup_1"
    test_subgroup2 = "test_subgroup_2"
    tmp_rasters = []

    @classmethod
    def setUpClass(cls):
        """Set up a temporary region and generate test raster maps."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        cls.runModule("r.mapcalc", expression="raster1 = row()", overwrite=True)
        cls.runModule("r.mapcalc", expression="raster2 = col()", overwrite=True)
        cls.runModule("r.mapcalc", expression="raster3 = row() + col()", overwrite=True)
        cls.tmp_rasters.extend(["raster1", "raster2", "raster3"])

    @classmethod
    def tearDownClass(cls):
        """Remove test rasters and restore the original region."""
        gs.run_command("g.remove", flags="f", type="raster", name=cls.tmp_rasters)
        cls.del_temp_region()

    def tearDown(self):
        """Remove the test group after each test to maintain isolation."""
        gs.run_command("g.remove", flags="f", type="group", name=self.test_group)

    def test_group_creation(self):
        """Verify that a raster group can be created and correctly listed. (-l flag)"""
        self.assertModule(
            "i.group",
            group=self.test_group,
            input="raster1,raster2,raster3",
        )
        groups = gs.read_command("i.group", group=self.test_group, flags="l")
        self.assertIn(self.test_group, groups)

    def test_add_remove_maps(self):
        """Test that rasters can be added to and removed from a group. (-r and -g flags)"""
        self.assertModule(
            "i.group",
            group=self.test_group,
            input="raster1,raster2",
        )
        self.assertModule(
            "i.group",
            group=self.test_group,
            input="raster3",
        )
        groups = gs.read_command("i.group", group=self.test_group, flags="g")
        self.assertIn("raster1", groups)
        self.assertIn("raster2", groups)
        self.assertIn("raster3", groups)

        self.assertModule(
            "i.group",
            group=self.test_group,
            input="raster2",
            flags="r",
        )
        groups = gs.read_command("i.group", group=self.test_group, flags="g")
        self.assertNotIn("raster2", groups)

    def test_subgroup_handling(self):
        """Test subgroup creation and listing of subgroup members.(-s flag)"""
        self.assertModule(
            "i.group",
            group=self.test_group,
            subgroup=self.test_subgroup1,
            input="raster1,raster2",
        )
        self.assertModule(
            "i.group",
            group=self.test_group,
            subgroup=self.test_subgroup2,
            input="raster3",
        )

        subgroup1 = gs.read_command(
            "i.group", group=self.test_group, subgroup=self.test_subgroup1, flags="s"
        )
        subgroup2 = gs.read_command(
            "i.group", group=self.test_group, subgroup=self.test_subgroup2, flags="s"
        )
        self.assertIn("raster1", subgroup1)
        self.assertIn("raster2", subgroup1)
        self.assertNotIn("raster3", subgroup1)
        self.assertNotIn("raster1", subgroup2)
        self.assertNotIn("raster2", subgroup2)
        self.assertIn("raster3", subgroup2)


if __name__ == "__main__":
    test()
