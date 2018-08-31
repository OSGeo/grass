"""
Name:       v.extract test
Purpose:    Tests v.extract and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
import os
from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import read_command

TABLE_1="""cat|MAJORRDS_|ROAD_NAME|MULTILANE|PROPYEAR|OBJECTID|SHAPE_LEN
1|1|NC-50|no|0|1|4825.369405
2|2|NC-50|no|0|2|14392.589058
3|3|NC-98|no|0|3|3212.981242
4|4|NC-50|no|0|4|13391.907552
"""
TABLE_2="""cat|onemap_pro|PERIMETER|GEOL250_|GEOL250_ID|GEO_NAME|SHAPE_area|SHAPE_len
1|963738.75|4083.97998|2|1|Zml|963738.608571|4083.979839
2|22189124|26628.261719|3|2|Zmf|22189123.2296|26628.261112
3|579286.875|3335.55835|4|3|Zml|579286.829631|3335.557182
4|20225526|33253|5|4|Zml|20225526.6368|33253.000508
5|450650720|181803.765625|6|5|Ybgg|450650731.029|181803.776199
"""

TABLE_3="""cat|MAJORRDS_|ROAD_NAME|MULTILANE|PROPYEAR|OBJECTID|SHAPE_LEN
1|1|NC-50|no|0|1|4825.369405
2|2|NC-50|no|0|2|14392.589058
3|3|NC-98|no|0|3|3212.981242
4|4|NC-50|no|0|4|13391.907552
5|5|NC-98|no|0|5|7196.001495
6|6||no|0|6|10185.513951
7|7|US-1|yes|0|7|13655.438596
8|8||no|0|8|797.901095
9|9|NC-98|no|0|9|14772.176241
"""
class TestRasterreport(TestCase):
    input="roadsmajor"
    output="testoutput"
    geology='geology'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
	
    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(cls):
        cls.runModule('g.remove', flags='f', type='vector', name=cls.output)

    def test_flagd(self):
        """Testing flag d """
        self.assertModule('v.extract', input=self.input, output=self.output, cats="1,2,3,4")
        category = read_command('v.db.select', map=self.output,
	                        separator='pipe')
        self.assertEqual(first=TABLE_1.replace('\n', os.linesep),
	                         second=category,
	                         msg="Attribute table has wrong entries")
 
    def test_cats2(self):
        """Testing cats=2 """
        self.assertModule('v.extract', input=self.geology, output=self.output, flags='d', cats="1,2,3,4,5")
        category = read_command('v.db.select', map=self.output,
	                        separator='pipe')
        self.assertEqual(first=TABLE_2.replace('\n', os.linesep),
	                         second=category,
	                         msg="Attribute table has wrong entries")
   
    def test_flagt(self):
        """Testing Falg T"""
        self.assertModule('v.extract', input=self.input, output=self.output, flags='t', cats=1 )

    def test_flatr(self):
        """Testing flag r """
        self.assertModule('v.extract', input=self.geology, output=self.output, flags='r', cats=1 )

    def  test_where(self):
        """Testing where"""
        self.assertModule('v.extract', input=self.input, output=self.output, flags='d', 
                            where="cat < 10")
        category = read_command('v.db.select', map=self.output,
	                        separator='pipe')
        self.assertEqual(first=TABLE_3.replace('\n', os.linesep),
	                         second=category,
	                         msg="Attribute table has wrong entries")
if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
