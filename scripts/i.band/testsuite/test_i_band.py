from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule, call_module

from grass.pygrass.raster import RasterRow

class TestBandsSystemDefined(TestCase):
    # note that full NC dataset is needed
    raster_map = "lsat7_2002_10"
    band_ref = "L7_1"

    def read_band_ref(self):
        with RasterRow(self.raster_map) as rast:
            band_ref = rast.info.band_reference

        return band_ref

    def test_band_ref_assign_not_current_mapset(self):
        # it is assumed that we are not in PERMANENT mapset
        module = SimpleModule('i.band', map=self.raster_map + '@PERMANENT',
                              band=self.band_ref)
        self.assertModuleFail(module)

    def test_band_ref_assign(self):
        # copy raster map to the current mapset
        call_module('g.copy',
                    raster='{m}@PERMANENT,{m}'.format(m=self.raster_map))

        module = SimpleModule('i.band', map=self.raster_map,
                              band=self.band_ref)
        self.assertModule(module)

        # check also using pygrass
        self.assertEqual(self.read_band_ref(), self.band_ref)

    def test_band_ref_dissociate(self):
        module = SimpleModule('i.band', operation='remove' , map=self.raster_map)
        self.assertModule(module)

        # check also using pygrass
        self.assertEqual(self.read_band_ref(), None)

if __name__ == '__main__':
    test()
