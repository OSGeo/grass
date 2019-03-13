"""
Created on Sun Jun 09 09:18:39 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestVDbDropTable(TestCase):
    """Test v.db.droptable script"""

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        run_command('g.copy', vector='roadsmajor,myroads')

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector"""
        run_command('g.remove', type='vector', name='myroads',
                    flags='f')

    def test_drop_table_check(self):
        """Drop table check, the column should still be in the table"""
        module = SimpleModule('v.db.droptable', map='myroads')
        self.assertModule(module)

        m = SimpleModule('db.tables', flags='p')
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), 'myroads')

    def test_drop_table_with_force(self):
        """Drop table with force, the column should not be in the table"""
        module = SimpleModule('v.db.droptable', map='myroads', flags='f')
        self.assertModule(module)

        m = SimpleModule('db.tables', flags='p')
        self.assertModule(m)
        self.assertNotRegexpMatches(decode(m.outputs.stdout), 'myroads')

if __name__ == '__main__':
    test()
