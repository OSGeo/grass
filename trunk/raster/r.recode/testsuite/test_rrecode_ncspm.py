# -*- coding: utf-8 -*-
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import read_command


rules1 = """
55:65:1
65:75:2
75:85:3
85:95:4
95:105:5
105:115:6
115:125:7
125:135:8
135:145:9
145:155:10
155:165:11
"""


rules2 = """
55.6:156:-0.5:0.5
"""

rules3 = """
0:1:0:255
"""

# landuse has 7 classes (in nc_spm_08 it is landclass96, not landuse96_28m) 
rules4 = """
*:3:1
4:5:2
6:*:3
"""


class TestNCMaps(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster='elevation@PERMANENT')
        cls.runModule('r.mapcalc', expression="random01 = rand(0, 1.)", seed=1, overwrite=True)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', type='raster', name=['random01', 'recoded'], flags='f')

    def test_formats_elevation(self):
        recode = SimpleModule('r.recode', input='elevation@PERMANENT', output='recoded',
                              rules='-', overwrite=True)
        recode.inputs.stdin = rules1
        self.assertModule(recode)
        info = 'min=1\nmax=11\ndatatype=CELL'
        self.assertRasterFitsInfo(raster='recoded', reference=info)

        recode.flags['d'].value = True
        self.assertModule(recode)
        info = 'min=1\nmax=11\ndatatype=DCELL'
        self.assertRasterFitsInfo(raster='recoded', reference=info)
        recode.flags['d'].value = False

        recode.inputs.stdin = rules2
        self.assertModule(recode)
        info = 'min=-0.5\nmax=0.5\ndatatype=FCELL'
        self.assertRasterFitsInfo(raster='recoded', reference=info, precision=1e-3)

        recode.flags['d'].value = True
        self.assertModule(recode)
        info = 'min=-0.5\nmax=0.5\ndatatype=DCELL'
        self.assertRasterFitsInfo(raster='recoded', reference=info, precision=1e-3)
        recode.flags['d'].value = False

    def test_formats_random(self):
        recode = SimpleModule('r.recode', input='random01', output='recoded',
                              rules='-', overwrite=True)
        recode.inputs.stdin = rules3
        self.assertModule(recode)
        category = read_command('r.category', map='recoded')
        n_cats = len(category.strip().split('\n'))
        if n_cats <= 2:
            self.fail(msg="Number of categories is <= 2 "
                          "which suggests input map values were read as integers.")

    def test_formats_landcover(self):
        recode = SimpleModule('r.recode', input='landuse@PERMANENT',
                              output='recoded', rules='-', overwrite=True)
        recode.inputs.stdin = rules4
        self.assertModule(recode)
        info = 'min=1\nmax=3\ndatatype=CELL'
        self.assertRasterFitsInfo(raster='recoded', reference=info)

if __name__ == '__main__':
    test()
