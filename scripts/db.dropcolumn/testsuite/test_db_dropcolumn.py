"""
Created on Sun Jun 07 19:08:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestDbDropColumn(TestCase):
    """Test db.dropcolumn script"""

    mapName = 'myroads'
    colName = 'SHAPE_LEN'

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        run_command('g.copy', vector='roadsmajor,myroads')

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector"""
        run_command('g.remove', type='vector', name=cls.mapName,
                    flags='f')

    def test_drop_column_check(self):
        """Drop column check, the column should still be in the table"""
        module = SimpleModule('db.dropcolumn', table=self.mapName,
                              column=self.colName)
        self.assertModule(module)

        m = SimpleModule('db.columns', table=self.mapName)
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), self.colName)

    def test_drop_column_with_force(self):
        """Drop column with force, the column should not be in the table"""
        module = SimpleModule('db.dropcolumn', table=self.mapName,
                              column=self.colName,
                              flags='f')
        self.assertModule(module)

        m = SimpleModule('db.columns', table=self.mapName)
        self.assertModule(m)
        self.assertNotRegexpMatches(decode(m.outputs.stdout), self.colName)

if __name__ == '__main__':
    test()
