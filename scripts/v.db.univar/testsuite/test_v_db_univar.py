"""
Created on Sun Jun 08 19:08:07 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command


class TestVDbUnivar(TestCase):
    """Test v.db.univar script"""

    mapName = 'elevation'
    columnName = 'heights'
    outputMap = 'samples'

    @classmethod
    def setUpClass(cls):
        """Use temp region"""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster', name=cls.mapName)
        cls.del_temp_region()

    def test_calculate(self):
        """run db.univar"""
        run_command('v.random', output=self.outputMap, n=100, overwrite='True')
        run_command('v.db.addtable', map=self.outputMap,
                    column="heights double precision")
        run_command('v.what.rast', map=self.outputMap, raster=self.mapName,
                    column=self.columnName)
        run_command('v.db.select', map=self.outputMap)

        module = SimpleModule('v.db.univar', map=self.outputMap,
                              column=self.columnName)
        self.assertModule(module)

if __name__ == '__main__':
    test()
