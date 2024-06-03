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

    def test_basic_group_label_keys(self):
        ref_dict = {f"L8_{band}": f"lsat7_2002_{band}0" for band in self.bands}
        group_info = gs.imagery.group_to_dict(self.group, full_info=False)
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(list(ref_dict.keys()), list(group_info.keys()))
        self.assertListEqual(
            list(ref_dict.values()), [val.split("@")[0] for val in group_info.values()]
        )

    def test_invalid_input(self):
        # Non existing group
        self.assertRaises(
            CalledModuleError, gs.imagery.group_to_dict, "non_existing_group"
        )
        # invalid dict_key
        self.assertRaises(
            CalledModuleError,
            gs.imagery.group_to_dict,
            self.group,
            **{"dict_key": "invalid_dict_key"},
        )

    def test_basic_group_map_keys(self):
        ref_dict = {f"lsat7_2002_{band}0": f"L8_{band}" for band in self.bands}
        group_info = gs.imagery.group_to_dict(
            self.group, dict_key="map_names", full_info=False
        )
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(
            list(ref_dict.keys()), [key.split("@")[0] for key in group_info.keys()]
        )
        self.assertListEqual(list(ref_dict.values()), list(group_info.values()))

    def test_basic_group_index_keys(self):
        ref_dict = {band: f"lsat7_2002_{band}0" for band in self.bands}
        group_info = gs.imagery.group_to_dict(
            self.group, dict_key="indices", full_info=False
        )
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(
            list(ref_dict.keys()), [key.split("@")[0] for key in group_info.keys()]
        )
        self.assertListEqual(list(ref_dict.values()), list(group_info.values()))

    def test_full_info_group_label_keys(self):
        group_info = gs.imagery.group_to_dict(self.group, full_info=True)
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(
            [f"L8_{band}" for band in self.bands], list(group_info.keys())
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
        self.runModule(
            "i.group", group=self.group, subgroup=self.subgroup, input=self.raster_maps
        )
        group_info = gs.imagery.group_to_dict(self.group, full_info=True)
        # Check that a dict is returned
        self.assertIsInstance(group_info, dict)
        self.assertListEqual(
            [f"L8_{band}" for band in self.bands], list(group_info.keys())
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


if __name__ == "__main__":
    test()
