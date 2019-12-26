"""
Created on Sun Jun 09 01:12:22 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestVDbDropColumn(TestCase):
    """Test v.db.dropcolumn script"""

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        run_command('g.copy', vector='roadsmajor,myroads')

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector"""
        run_command('g.remove', type='vector', name='myroads',
                    flags='f')

    def test_drop_single_column_check(self):
        """Drop column to the attribute table"""
        module = SimpleModule('v.db.dropcolumn', map='myroads',
                              columns='SHAPE_LEN')
        self.assertModule(module)

        m = SimpleModule('v.info', map='myroads', flags='c')
        self.assertModule(m)
        self.assertNotRegexpMatches(decode(m.outputs.stdout), 'SHAPE_LEN')

if __name__ == '__main__':
    test()
