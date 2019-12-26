"""
Created on Sun Jun 08 22:14:26 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command


class TestVDbUpdate(TestCase):
    """Test v.db.update script"""

    mapName = 'mygeodetic_pts'

    @classmethod
    def setUpClass(cls):
        """Copy vect"""
        run_command('g.copy', vector='geodetic_pts,mygeodetic_pts')

    @classmethod
    def tearDownClass(cls):
        """Remove vector"""
        run_command('v.db.dropcolumn', map=cls.mapName, columns="zval")
        run_command('g.remove', flags='f', type='vector', name=cls.mapName)

    def test_update(self):
        """update value test"""
        run_command('v.db.addcolumn', map=self.mapName,
                    column="zval double precision")

        module = SimpleModule('v.db.update', map=self.mapName, column='zval',
                              query_column="CAST(z_value AS double precision)",
                              where="z_value <> 'N/A'")
        self.assertModule(module)

if __name__ == '__main__':
    test()
