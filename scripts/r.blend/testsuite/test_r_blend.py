"""
Created on Sun Jun 07 21:42:39 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import run_command


class TestRBlend(TestCase):
    """Test r.blend script"""

    map1 = 'aspect'
    map2 = 'elevation'
    temp1 = 'elev_shade_blend.r'
    temp2 = 'elev_shade_blend.g'
    temp3 = 'elev_shade_blend.b'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.map1, flags='p')
        run_command('d.mon', start='png')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=(cls.temp1, cls.temp2, cls.temp3))
        cls.del_temp_region()
        run_command('d.mon', stop='png')

    def test_blend(self):
        """blends color test"""
        module = SimpleModule('r.blend', first=self.map1, second=self.map2,
                              output='elev_shade_blend')
        self.assertModule(module)

        run_command('d.rgb', red=self.temp1, green=self.temp2,
                    blue=self.temp3)

if __name__ == '__main__':
    test()
