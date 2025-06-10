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

        groups_shell = gs.read_command("i.group", group=self.test_group, format="shell")
        self.assertEqual(groups_shell, groups)

        self.assertModule(
            "i.group",
            group=self.test_group,
            input="raster2",
            flags="r",
        )
        groups = gs.read_command("i.group", group=self.test_group, flags="g")
        self.assertNotIn("raster2", groups)

        groups_shell = gs.read_command("i.group", group=self.test_group, format="shell")
        self.assertEqual(groups_shell, groups)

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

    def test_list_files_shell(self):
        """Test listing of (sub)group files in shell format."""
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

        # Test listing of test_subgroup1 files in shell format.
        subgroup = gs.read_command(
            "i.group",
            group=self.test_group,
            subgroup=self.test_subgroup1,
            flags="l",
            format="shell",
        )

        self.assertIn("raster1", subgroup)
        self.assertIn("raster2", subgroup)
        self.assertNotIn("raster3", subgroup)

        # Test listing of test_subgroup2 files in shell format.
        subgroup = gs.read_command(
            "i.group",
            group=self.test_group,
            subgroup=self.test_subgroup2,
            flags="l",
            format="shell",
        )

        self.assertNotIn("raster1", subgroup)
        self.assertNotIn("raster2", subgroup)
        self.assertIn("raster3", subgroup)

        # Test listing of group files in shell format.
        group = gs.read_command(
            "i.group",
            group=self.test_group,
            flags="l",
            format="shell",
        )

        self.assertIn("raster1", group)
        self.assertIn("raster2", group)
        self.assertIn("raster3", group)

    def test_list_files_json(self):
        """Test listing of (sub)group files in json format."""
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

        # Test listing of test_subgroup_1 files in json format.
        maps = gs.parse_command(
            "i.group",
            group=self.test_group,
            subgroup=self.test_subgroup1,
            flags="l",
            format="json",
        )

        self.assertTrue(any("raster1" in m for m in maps))
        self.assertTrue(any("raster2" in m for m in maps))
        self.assertTrue(all("raster3" not in m for m in maps))

        # Test listing of test_subgroup2 files in json format.
        maps = gs.parse_command(
            "i.group",
            group=self.test_group,
            subgroup=self.test_subgroup2,
            flags="l",
            format="json",
        )

        self.assertTrue(all("raster1" not in m for m in maps))
        self.assertTrue(all("raster2" not in m for m in maps))
        self.assertTrue(any("raster3" in m for m in maps))

        # Test listing of group files in json format.
        maps = gs.parse_command(
            "i.group",
            group=self.test_group,
            flags="l",
            format="json",
        )

        self.assertTrue(any("raster1" in m for m in maps))
        self.assertTrue(any("raster2" in m for m in maps))
        self.assertTrue(any("raster3" in m for m in maps))

    def test_list_subgroups_shell(self):
        """Test listing of subgroups in group using shell format."""
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

        subgroups = gs.read_command(
            "i.group",
            group=self.test_group,
            flags="s",
            format="shell",
        )

        self.assertIn(self.test_subgroup1, subgroups)
        self.assertIn(self.test_subgroup2, subgroups)

    def test_list_subgroups_json(self):
        """Test listing of subgroups in group using json format."""
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

        subgroups = gs.parse_command(
            "i.group",
            group=self.test_group,
            flags="s",
            format="json",
        )
        expected_subgroups = [self.test_subgroup1, self.test_subgroup2]

        self.assertEqual(expected_subgroups, subgroups)


if __name__ == "__main__":
    test()
