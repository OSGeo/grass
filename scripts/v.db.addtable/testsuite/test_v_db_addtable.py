"""
Created on Sun Jun 09 12:01:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestVDbAddTable(TestCase):
    """Test v.db.addtable script"""

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        run_command('g.copy', vector='roadsmajor,myroads')

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector"""
        run_command('g.remove', type='vector', name='myroads',
                    flags='f')

    def test_add_single_columned_table_check(self):
        """Add a new attribute table with single column to layer 2"""
        module = SimpleModule('v.db.addtable', map='myroads',
                              columns='slope double precision', layer=2)
        self.assertModule(module)

        run_command('v.db.connect', flags='p', map='myroads')

        m = SimpleModule('v.info', map='myroads', flags='c')
        self.assertModule(m)
        self.assertNotRegexpMatches(decode(m.outputs.stdout), 'slope')

        m = SimpleModule('v.info', map='myroads', flags='c', layer=2)
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), 'slope')

if __name__ == '__main__':
    test()
