"""
Created on Sun Jun 07 21:57:07 2018

@author: Sanjeet Bhatti
"""
import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import run_command


class TestRFillNulls(TestCase):
    """Test r.fillnulls script"""

    module_dir = os.path.dirname(os.path.dirname(__file__))
    module = os.path.join(module_dir, 'r.fillnulls')
    mapName = 'elevation'
    expression = 'elevation_filt = if(elevation > 130, \
    null(), elevation)'
    mapNameCalc = 'elevation_filt'
    mapComplete = 'elevation_complete'
    values = 'null_cells=0'

    def setUp(self):
        """Create maps in a small region."""
        self.use_temp_region()
        self.runModule('g.region', res=200, raster=self.mapName, flags='ap')
        run_command('r.mapcalc', expression=self.expression)

    def tearDown(self):
        """Remove temporary region"""
        self.runModule('g.remove', flags='f', type='raster',
                       name=(self.mapNameCalc, self.mapComplete))
        self.del_temp_region()

    def test_rst(self):
        module = SimpleModule(self.module, input=self.mapNameCalc,
                              output=self.mapComplete, segmax=1200,
                              npmin=100, tension=150)
        self.assertModule(module)
        self.assertRasterFitsUnivar(raster=self.mapComplete,
                                    reference=self.values)

    def test_bspline(self):
        module = SimpleModule(self.module, input=self.mapNameCalc,
                              output=self.mapComplete, method='bicubic')
        self.assertModule(module)
        self.assertRasterFitsUnivar(raster=self.mapComplete,
                                    reference=self.values)


if __name__ == '__main__':
    test()
