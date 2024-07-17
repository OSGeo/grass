from grass.exceptions import CalledModuleError
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.script as gs


class TestImageryGroupToDict(TestCase):
    """Tests function `group_to_dict` that returns raster maps
    from an imagery group and their metadata."""

    @classmethod
    def setUpClass(cls):
        cls.bands = [1, 2, 3]
        cls.raster_maps = [f"lsat7_2002_{band}0" for band in cls.bands]
        cls.group = "L8_group"
        cls.subgroup = "L8_group_subgroup"
        # Create input maps with label and group
        for band in cls.bands:
            cls.runModule(
                "g.copy", raster=[f"lsat7_2002_{band}0", f"lsat7_2002_{band}0"]
            )
            cls.runModule(
                "r.support", map=f"lsat7_2002_{band}0", semantic_label=f"L8_{band}"
            )
        cls.runModule("i.group", group=cls.group, input=cls.raster_maps)

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", type="raster", name=cls.raster_maps, flags="f")
        cls.runModule("g.remove", type="group", name=cls.group, flags="f")

    def test_basic_group_dict_defaults(self):
        """Test with semantic labels as keys and map names as values (defaults)"""
        ref_dict = {f"L8_{band}": f"lsat7_2002_{band}0" for band in self.bands}
        group_info = gs.imagery.group_to_dict(self.group)
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(list(ref_dict.keys()), list(group_info.keys()))
        self.assertListEqual(
            list(ref_dict.values()), [val.split("@")[0] for val in group_info.values()]
        )

    def test_non_existing_group(self):
        """Test that function fails if group does not exist"""
        # Non existing group
        self.assertRaises(
            CalledModuleError, gs.imagery.group_to_dict, "non_existing_group"
        )

    def test_invalid_dict_key(self):
        """Test that function fails if invalid keys are requested"""
        self.assertRaises(
            ValueError,
            gs.imagery.group_to_dict,
            self.group,
            dict_keys="invalid_dict_key",
        )

    def test_invalid_dict_value(self):
        """Test that function fails if invalid values are requested"""
        self.assertRaises(
            ValueError,
            gs.imagery.group_to_dict,
            self.group,
            dict_values="invalid_dict_value",
        )

    def test_missing_subgroup(self):
        """Test that empty dict is returned if subgroup does not exist"""
        group_info = gs.imagery.group_to_dict(
            self.group, subgroup="non_existing_subgroup"
        )

        # Check that an empty dict is returned
        self.assertDictEqual(group_info, {})

    def test_basic_group_map_keys(self):
        """Test with map_names as keys and semantic_labels as values"""
        ref_dict = {f"lsat7_2002_{band}0": f"L8_{band}" for band in self.bands}
        group_info = gs.imagery.group_to_dict(
            self.group, dict_keys="map_names", dict_values="semantic_labels"
        )
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(
            list(ref_dict.keys()), [key.split("@")[0] for key in group_info.keys()]
        )
        self.assertListEqual(list(ref_dict.values()), list(group_info.values()))

    def test_basic_group_index_keys(self):
        """Test with indices as keys and mapnames as values"""
        ref_dict = {str(band): f"lsat7_2002_{band}0" for band in self.bands}
        group_info = gs.imagery.group_to_dict(self.group, dict_keys="indices")
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(list(ref_dict.keys()), list(group_info.keys()))
        self.assertListEqual(
            list(ref_dict.values()),
            [val.split("@")[0] for val in group_info.values()],
        )

    def test_full_info_group_label_keys(self):
        """Test with semantic labels as keys and full map metadata as values"""
        group_info = gs.imagery.group_to_dict(self.group, dict_values="metadata")
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(
            [f"L8_{band}" for band in self.bands],
            [key.split("@")[0] for key in group_info.keys()],
        )
        for band in self.bands:
            # Take some metadata keys from raster_info
            for metadata_key in [
                "north",
                "nsres",
                "cols",
                "datatype",
                "map",
                "date",
                "semantic_label",
                "comments",
            ]:
                self.assertIn(metadata_key, group_info[f"L8_{band}"])

    def test_full_info_group_label_keys_subgroup(self):
        """Test with map names as keys and full map metadata as values"""
        metadata_keys = {
            "north",
            "nsres",
            "cols",
            "datatype",
            "map",
            "date",
            "semantic_label",
            "comments",
        }
        self.runModule(
            "i.group", group=self.group, subgroup=self.subgroup, input=self.raster_maps
        )
        group_info = gs.imagery.group_to_dict(
            self.group,
            subgroup=self.subgroup,
            dict_keys="map_names",
            dict_values="metadata",
        )
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(
            [f"lsat7_2002_{band}0" for band in self.bands],
            [key.split("@")[0] for key in group_info.keys()],
        )
        for key, val in group_info.items():
            # Check keys
            self.assertTrue(key.startswith("lsat7_2002_"))
            # Check values
            self.assertIsInstance(val, dict)
            # Take some metadata keys from raster_info
            self.assertTrue(metadata_keys.issubset(set(val.keys())))


if __name__ == "__main__":
    test()
