import os
import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestITarget(TestCase):
    """Regression tests for the i.target GRASS GIS module."""

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", n=50, s=0, e=50, w=0, rows=5, cols=5)
        cls.runModule("r.mapcalc", expression="test_base = 100", overwrite=True)
        cls.runModule("r.mapcalc", expression="test_extra = 200", overwrite=True)
        cls.groups = ["test_group1", "test_group2", "test_group3"]
        for group in cls.groups:
            cls.runModule("i.group", group=group, input="test_base")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", type="raster", name="test_base,test_extra", flags="f")
        cls.runModule("g.remove", type="group", name=cls.groups, flags="f")

    def _target_path(self, group):
        """Return full filesystem path to group's TARGET file."""
        env = gs.gisenv()
        return os.path.join(
            env["GISDBASE"],
            env["LOCATION_NAME"],
            env["MAPSET"],
            "group",
            group,
            "TARGET",
        )

    def _get_target_values(self, group):
        """Return (location, mapset) tuple from group's TARGET file."""
        with open(self._target_path(group)) as f:
            return (f.readline().strip(), f.readline().strip())

    def test_current_location_flag(self):
        """Verify -c flag creates target with current environment values."""
        group = self.groups[0]
        target_file = self._target_path(group)

        if os.path.exists(target_file):
            os.remove(target_file)

        self.assertModule("i.target", flags="c", group=group)
        self.assertTrue(os.path.exists(target_file))

        current_env = gs.gisenv()
        loc, mset = self._get_target_values(group)
        self.assertEqual(loc, current_env["LOCATION_NAME"])
        self.assertEqual(mset, current_env["MAPSET"])

    def test_specific_location_mapset(self):
        """Verify explicit location/mapset parameters set correct values."""
        group = self.groups[0]
        current_env = gs.gisenv()

        self.assertModule(
            "i.target",
            group=group,
            location=current_env["LOCATION_NAME"],
            mapset=current_env["MAPSET"],
        )

        loc, mset = self._get_target_values(group)
        self.assertEqual(loc, current_env["LOCATION_NAME"])
        self.assertEqual(mset, current_env["MAPSET"])

    def test_independent_group_targets(self):
        """Verify target settings don't propagate between groups.
        Ensures configuration changes in one group don't affect others."""
        group1, group2 = self.groups[0], self.groups[1]
        self.assertModule("i.target", group=group1, location="loc1", mapset="mapset1")
        self.assertModule("i.target", group=group2, location="loc2", mapset="mapset2")

        loc1, mset1 = self._get_target_values(group1)
        loc2, mset2 = self._get_target_values(group2)
        self.assertNotEqual((loc1, mset1), (loc2, mset2))

    def test_target_survives_group_modification(self):
        """Ensure modifying a group's composition does not reset its target settings."""
        group = self.groups[1]
        self.assertModule("i.target", flags="c", group=group)

        self.runModule("i.group", group=group, input="test_base", flags="r")
        self.runModule("i.group", group=group, input="test_extra")

        current_env = gs.gisenv()
        loc, mset = self._get_target_values(group)
        self.assertEqual(loc, current_env["LOCATION_NAME"])
        self.assertEqual(mset, current_env["MAPSET"])

    def test_subgroups_persist_after_target_change(self):
        """Verify subgroup existence after target modification."""
        group = self.groups[2]
        subgroup = "test_sub"

        self.runModule("i.group", group=group, subgroup=subgroup, input="test_base")
        self.assertModule(
            "i.target", group=group, location="new_loc", mapset="PERMANENT"
        )

        subgroups = gs.read_command("i.group", flags="sg", group=group).split()
        self.assertIn(subgroup, subgroups)


if __name__ == "__main__":
    test()
