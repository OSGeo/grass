# -*- coding: utf-8 -*-
"""
Test of r3.flow

@author Anna Petrasova
"""
import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

seeds = """
84.80609404|35.19733594|39.43321996
14.42915927|56.86951467|22.42338987
29.06094033|78.06029074|39.31707858
64.95524796|50.76614609|12.02301418
75.47677891|18.36008965|29.362122
88.84231714|31.23108675|34.33555293
41.13822083|64.70413255|25.85158957
14.15768282|45.26556161|10.63049231
69.36315244|2.92994235|39.63663467
95.88028571|0.80210167|28.57206445
"""

flowaccum = """
n=480
null_cells=0
cells=480
min=0
max=89
range=89
mean=6.38333333333333
mean_of_abs=6.38333333333333
stddev=11.3061070026581
variance=127.828055555556
coeff_var=177.119169754436
sum=3064
"""

class FlowlineTest(TestCase):

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule("g.region", res=10, res3=10, n=80, s=0, w=0, e=120, b=0, t=50)
        cls.runModule("r3.mapcalc", expression="map_1 = 100")
        cls.runModule("r3.mapcalc", expression="map_2 = -20")
        cls.runModule("r3.mapcalc", expression="map_3 = 0.01")
        cls.runModule("r3.mapcalc", expression="map_4 = col() + row() + depth()")
        cls.runModule("r3.mapcalc", expression="map_5 = col() * col() + row() * row() + depth() * depth()")
        cls.runModule('v.in.ascii', input='-', output='test_seeds', z=3, flags='zt',
                      stdin=seeds)

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()
        cls.runModule('g.remove', flags='f', type='raster_3d', name=','.join(['map_1', 'map_2', 'map_3', 'map_4', 'map_5', 'test_flowaccum']))
        cls.runModule('g.remove', flags='f', type='vector', name=','.join(['test_flowline', 'test_seeds']))
        os.remove('./data/flowline_tmp.ascii')

    def test_interpolation(self):
        self.assertModuleKeyValue('test.r3flow', test='interpolation',
                                  coordinates=[100, 55, 11], input=['map_1', 'map_2', 'map_3'],
                                  reference={'return': 0, 'values': [100, -20, 0.01]},
                                  precision=1e-10, sep='=')
        self.assertModuleKeyValue('test.r3flow', test='interpolation',
                                  coordinates=[5, 5, 5], input=['map_1', 'map_2', 'map_3'],
                                  reference={'return': 0, 'values': [100, -20, 0.01]},
                                  precision=1e-10, sep='=')
        self.assertModuleKeyValue('test.r3flow', test='interpolation',
                                  coordinates=[10, 10, 60], input=['map_1', 'map_2', 'map_3'],
                                  reference={'return': -1},
                                  precision=1e-10, sep='=')
        self.assertModuleKeyValue('test.r3flow', test='interpolation',
                                  coordinates=[25, 69, 17], input=['map_4', 'map_4', 'map_4'],
                                  reference={'return': 0, 'values': [7.8, 7.8, 7.8]},
                                  precision=1e-10, sep='=')
        self.assertModuleKeyValue('test.r3flow', test='interpolation',
                                  coordinates=[81, 30, 25], input=['map_4', 'map_4', 'map_4'],
                                  reference={'return': 0, 'values': [18.1, 18.1, 18.1]},
                                  precision=1e-10, sep='=')

    def test_flowlines(self):
        self.assertModule('r3.flow', input='map_5', flowline='test_flowline',
                          seed_points='test_seeds', flowaccumulation='test_flowaccum',
                          direction='down')
        self.runModule('v.out.ascii', input='test_flowline',
                       format='standard', output='./data/flowline_tmp.ascii',
                       precision=6)
        self.assertVectorAsciiEqualsVectorAscii(actual='./data/flowline_tmp.ascii',
                                                reference='./data/flowline.ascii')
        self.assertRaster3dFitsUnivar('test_flowaccum', reference=flowaccum, precision=1e-6)


if __name__ == '__main__':
    test()
