"""
Created on Sun Jun 07 21:01:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command


class TestDbUnivar(TestCase):
    """Test db.univar script"""

    columnName = 'heights'
    mapName = 'samples'

    @classmethod
    def setUpClass(cls):
        """Use temp region"""
        cls.use_temp_region()
        cls.runModule('g.region', raster='elevation', flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.runModule('g.remove', flags='f', type='raster', name='elevation')
        cls.del_temp_region()

        run_command('v.db.droptable', map='samples', flags='f')

    def test_calculate(self):
        """run db.univar"""
        run_command('v.random', output=self.mapName, n=100, overwrite='True')
        run_command('v.db.addtable', map=self.mapName,
                    column="heights double precision")
        run_command('v.what.rast', map=self.mapName, raster='elevation',
                    column=self.columnName)
        run_command('v.db.select', map=self.mapName)

        module = SimpleModule('db.univar', table=self.mapName,
                              column=self.columnName)
        self.assertModule(module)

if __name__ == '__main__':
    test()
