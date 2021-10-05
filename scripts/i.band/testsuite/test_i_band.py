from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule, call_module

from grass.script.core import tempname
from grass.pygrass.gis import Mapset
from grass.pygrass.raster import RasterRow


class TestBandsSystemDefined(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.map = tempname(10)
        cls.bandref = "The_Doors"
        cls.mapset = Mapset()
        cls.use_temp_region()
        cls.runModule("g.region", n=1, s=0, e=1, w=0, res=1)
        cls.runModule("r.mapcalc", expression="{} = 1".format(cls.map))

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule("g.remove", flags="f", type="raster", name=cls.map)

    def read_band_ref(self):
        with RasterRow(self.map) as rast:
            band_ref = rast.info.bandref

        return band_ref

    def test_band_ref_assign_not_current_mapset(self):
        if not self.mapset == "PERMANENT":
            self.mapset.name = "PERMANENT"
            a_map = self.mapset.glist(type="raster")[0]
            module = SimpleModule("i.band", map=a_map, band=self.bandref)
            self.assertModuleFail(module)

    def test_band_ref_assign(self):
        module = SimpleModule("i.band", map=self.map, band=self.bandref)
        self.assertModule(module)

        # check also using pygrass
        self.assertEqual(self.read_band_ref(), self.bandref)

    def test_band_ref_dissociate(self):
        module = SimpleModule("i.band", operation="remove", map=self.map)
        self.assertModule(module)

        # check also using pygrass
        self.assertEqual(self.read_band_ref(), None)


if __name__ == "__main__":
    test()
