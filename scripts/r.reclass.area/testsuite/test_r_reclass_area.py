"""
Name:        r.reclass.area  test
Purpose:    Tests  r.reclass.area.

Author:     Shubham Sharma, Google Code-in 2018
Copyright:  (C) 2018 by Shubham Sharma and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestReclassArea(TestCase):
    input = 'geology_30m'
    output = 'reclassarea'
    value = '20'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output + 'Greater')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output + 'Lesser')

    def test_reclassaeaGreater(self):
        """Testing r.reclass.area with greater"""
        self.assertModule('r.reclass.area', input=self.input, output=self.output + 'Greater',
                          value=self.value, mode='greater', method='reclass')
        self.assertRasterMinMax(map=self.output + 'Greater', refmin=200, refmax=1000,
                                msg="Range of data: min = 200  max = 1000")

    def test_reclassareaLesser(self):
        """Testing r.reclass.area with lesser"""
        self.assertModule('r.reclass.area', input=self.input, output=self.output + 'Lesser',
                          value=self.value, mode='lesser', method='reclass')
        self.assertRasterMinMax(map=self.output + 'Lesser', refmin=900, refmax=1000,
                                msg="Range of data: min = 900  max = 1000")

if __name__ == '__main__':
    test()
