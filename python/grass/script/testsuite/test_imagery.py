from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.script as gs


class TestImageryGroupToDict(TestCase):
    """Tests function `group_to_dict` that returns raster maps
    from an imagery group and their metadata."""

    @classmethod
    def setUpClass(cls):
        cls.bands = [1, 2, 3]
        cls.raster_maps = ",".join([f"lsat7_2000_{band}" for band in cls.bands])
        cls.group = "L8_group"
        cls.subgroup = "L8_group_subgroup"
        for band in cls.bands:
            cls.runModule("g.copy", raster=f"lsat7_2000_{band}0,lsat7_2000_{band}0")
            cls.runModule(
                "r.support", raster=f"lsat7_2000_{band}", semantic_label=f"L8_{band}"
            )
        cls.runModule("i.group", group="L8_group", input=cls.raster_maps)

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", type="raster", name=cls.raster_maps, flags="f")
        cls.runModule("g.remove", type="group", name=cls.group, flags="f")

    def test_basic_group_label_keys(self):
        ref_dict = {f"L8_{band}": f"lsat7_2000_{band}" for band in self.bands}
        group_info = gs.imagery.group_to_dict(self.group, full_info=False)
        self.assertIsInstance(dict, group_info)
        self.assertDictEqual(ref_dict, group_info)

    def test_basic_group_map_keys(self):
        ref_dict = {f"lsat7_2000_{band}": f"L8_{band}" for band in self.bands}
        group_info = gs.imagery.group_to_dict(
            self.group, dict_key=None, full_info=False
        )
        self.assertIsInstance(dict, group_info)
        self.assertDictEqual(ref_dict, group_info)

    def test_full_info_group_label_keys(self):
        group_info = gs.imagery.group_to_dict(self.group, full_info=False)
        self.assertIsInstance(dict, group_info)
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
