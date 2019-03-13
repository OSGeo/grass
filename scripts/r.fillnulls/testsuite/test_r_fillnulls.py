"""
Created on Sun Jun 07 21:57:07 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import run_command


class TestRFillNulls(TestCase):
    """Test r.fillnulls script"""

    mapName = 'elev_srtm_30m'
    expression = 'elev_srtm_30m_filt = if(elev_srtm_30m < 50.0, \
    null(), elev_srtm_30m)'
    mapNameCalc = 'elev_srtm_30m_filt'
    mapComplete = 'elev_srtm_30m_complete'
    values = 'null_cells=0'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=(cls.mapNameCalc, cls.mapComplete))
        cls.del_temp_region()

    def test_fill_nulls(self):
        """Fill nulls test"""
        run_command('r.mapcalc', expression=self.expression)

        module = SimpleModule('r.fillnulls', input=self.mapNameCalc,
                              output=self.mapComplete, tension=20)
        self.assertModule(module)

        self.assertRasterFitsUnivar(raster=self.mapComplete,
                                    reference=self.values)

if __name__ == '__main__':
    test()
