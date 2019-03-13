"""
Created on Sun Jun 09 11:28:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestVDbAddColumn(TestCase):
    """Test v.db.addcolumn script"""

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        run_command('g.copy', vector='roadsmajor,myroads')

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector"""
        run_command('g.remove', type='vector', name='myroads',
                    flags='f')

    def test_add_single_column_check(self):
        """Add column to the attribute table"""
        module = SimpleModule('v.db.addcolumn', map='myroads',
                              columns='slope double precision')
        self.assertModule(module)

        m = SimpleModule('v.info', map='myroads', flags='c')
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), 'slope')

    def test_add_two_columns_check(self):
        """Add two column to the attribute table"""
        module = SimpleModule('v.db.addcolumn', map='myroads',
                              columns='slope_2 double precision, \
                              myname varchar(15)')
        self.assertModule(module)

        m = SimpleModule('v.info', map='myroads', flags='c')
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), 'slope_2')
        self.assertRegexpMatches(decode(m.outputs.stdout), 'myname')

if __name__ == '__main__':
    test()
