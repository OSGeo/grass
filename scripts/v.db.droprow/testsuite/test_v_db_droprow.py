"""
Created on Sun Jun 09 01:52:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestVDbDropRow(TestCase):
    """Test v.db.droprow script"""

    mapName = 'elevation'
    inputMap = 'rand5k_elev'
    outputMap = 'rand5k_elev_filt'
    values = 'min=56.12\nmax=155.157'

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

        cls.runModule('g.remove', flags='f', type='vector',
                      name=(cls.inputMap, cls.outputMap))

    def test_drop_row_check(self):
        """Drop vector object from a vector map"""
        run_command('v.random', output=self.inputMap, n=5000)
        run_command('v.db.addtable', map=self.inputMap,
                    column="elevation double precision")
        run_command('v.what.rast', map=self.inputMap,
                    raster=self.mapName, column=self.mapName)

        module = SimpleModule('v.db.droprow', input=self.inputMap,
                              output=self.outputMap, where='elevation IS NULL')
        self.assertModule(module)

        self.assertVectorFitsUnivar(map=self.outputMap, column='elevation',
                                    reference=self.values, precision=5)

if __name__ == '__main__':
    test()
