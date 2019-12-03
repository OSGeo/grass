"""
Name:       i.vi test
Purpose:    Tests i.vi and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
                License (>=v2). Read the file COPYING that comes with GRASS
                for details.
"""
from grass.gunittest.case import TestCase


class TestReport(TestCase):
    blue = 'lsat5_1987_10@landsat'
    green = 'lsat5_1987_20@landsat'
    red = 'lsat5_1987_30@landsat'
    nir = 'lsat5_1987_40@landsat'

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.runModule("g.region", raster='lsat5_1987_30@landsat')
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", flags='f', type="raster", name="ipvi")
        cls.runModule("g.remove", flags='f', type="raster", name="ndwi")
        cls.runModule("g.remove", flags='f', type="raster", name="dvi")
        cls.runModule("g.remove", flags='f', type="raster", name="sr")
        cls.runModule("g.remove", flags='f', type="raster", name="evi")
        cls.runModule("g.remove", flags='f', type="raster", name="evi2")
        cls.runModule("g.remove", flags='f', type="raster", name="gari")
        cls.runModule("g.remove", flags='f', type="raster", name="gemi")
        cls.del_temp_region()

    def test_vinameipvi(self):
        """Testing viname ipvi"""
        map_output = 'ipvi'
        self.assertModule('i.vi', red=self.red, nir=self.nir,
                          output=map_output, viname='ipvi')
        self.assertRasterMinMax(map=map_output, refmin=0.0454545454545,
                                refmax=0.9066667,
                                msg="ipvi in degrees must be between 0.0454545 and 0.9066667")

    def test_vinamendwi(self):
        """Testing viname ndwi"""
        map_output = 'ndwi'
        self.assertModule('i.vi', red=self.red, green=self.green, nir=self.nir, viname='ndwi',
                          output=map_output)
        self.assertRasterMinMax(map=map_output, refmin=-0.71, refmax=0.93,
                                msg="ndwi in percent must be between -0.71 and 0.93")

    def test_vinamedvi(self):
        """Testing viname dvi"""
        map_output = 'dvi'
        self.assertModule('i.vi', red=self.red, nir=self.nir, viname='dvi',
                          output=map_output)
        self.assertRasterMinMax(map=map_output, refmin=-0.33, refmax=0.56,
                                msg="dvi in percent must be between -0.32 and 0.52")

    def test_vinamesr(self):
        """Testing viname sr"""
        map_output = 'sr'
        self.assertModule('i.vi', red=self.red, nir=self.nir, blue=self.blue,
                          output=map_output, viname='sr')
        self.assertRasterMinMax(map=map_output, refmin=0.04, refmax=9.73,
                                msg="sr in percent must be between 0.04 and 9.72")

    def test_vinameevi(self):
        """Testing viname evi"""
        map_output = 'evi'
        self.assertModule('i.vi', red=self.red, nir=self.nir, blue=self.blue,
                          output=map_output, viname='evi')
        self.assertRasterMinMax(map=map_output, refmin=-8.12414050428e+16,
                                refmax=4.45061610234e+17,
                                msg="evi in degrees must be between -8.12 and 4.45061610234e+17")

    def test_vinameevi2(self):
        """Testing viname evi2"""
        map_output = 'evi2'
        self.assertModule('i.vi', red=self.red, nir=self.nir,
                          output=map_output, viname='evi2')
        self.assertRasterMinMax(map=map_output, refmin=-0.33, refmax=0.74,
                                msg="evi2 in degrees must be between -0.33 and 0.74")

    def test_vinamegari(self):
        """Testing viname gari"""
        map_output = 'gari'
        self.assertModule('i.vi', red=self.red, nir=self.nir, blue=self.blue,
                          green=self.green, output=map_output, viname='gari')
        self.assertRasterMinMax(map=map_output, refmin=-1.58244128083e+17,
                                refmax=float('inf'),
                                msg="gari in degrees must be between -1.58")

    def test_vinamegemi(self):
        """Testing viname gemi"""
        map_output = 'gemi'
        self.assertModule('i.vi', red=self.red, nir=self.nir,
                          output=map_output, viname='gemi')
        self.assertRasterMinMax(map=map_output, refmin=-221.69, refmax=0.97,
                                msg="gemi in degrees must be between -221.69 and 0.97")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
