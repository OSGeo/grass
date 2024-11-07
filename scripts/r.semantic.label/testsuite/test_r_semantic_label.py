from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import tempname
from grass.pygrass.gis import Mapset
from grass.pygrass.raster import RasterRow


class TestSemanticLabelsSystemDefined(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.semantic_label = "The_Doors"
        cls.mapset = Mapset()
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

    def read_semantic_label(self):
        with RasterRow(self.map) as rast:
            return rast.info.semantic_label

    def test_semantic_label_assign_not_current_mapset(self):
        if self.mapset != "PERMANENT":
            self.mapset.name = "PERMANENT"
            a_map = self.mapset.glist(type="raster")[0]
            module = SimpleModule(
                "r.semantic.label", map=a_map, semantic_label=self.semantic_label
            )
            self.assertModuleFail(module)

    def test_semantic_label_assign(self):
        module = SimpleModule(
            "r.semantic.label", map=self.map, semantic_label=self.semantic_label
        )
        self.assertModule(module)

        # check also using pygrass
        self.assertEqual(self.read_semantic_label(), self.semantic_label)

    def test_semantic_label_print(self):
        semantic_label = "S2_1"
        semantic_label_desc = "S2 Visible (Coastal/Aerosol)"
        module = SimpleModule(
            "r.semantic.label",
            map=self.map,
            semantic_label=semantic_label,
        )
        self.assertModule(module)

        module = SimpleModule("r.semantic.label", map=self.map, operation="print")
        self.assertModule(module)

        self.assertEqual(module.outputs.stdout.strip(), semantic_label_desc)

    def test_semantic_label_dissociate(self):
        module = SimpleModule("r.semantic.label", operation="remove", map=self.map)
        self.assertModule(module)

        # check also using pygrass
        self.assertEqual(self.read_semantic_label(), None)


if __name__ == "__main__":
    test()
