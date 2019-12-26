"""
Created on Sun Jun 07 19:08:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode


class TestVDbRenameColumn(TestCase):
    """Test v.db.renamecolumn script"""

    @classmethod
    def setUpClass(cls):
        """Copy vector."""
        run_command('g.copy', vector='roadsmajor,myroads')

    @classmethod
    def tearDownClass(cls):
        """Remove copied vector"""
        run_command('g.remove', type='vector', name='myroads',
                    flags='f')

    def test_rename_column(self):
        """Renaming a column"""
        module = SimpleModule('v.db.renamecolumn', map='myroads',
                              column=('ROAD_NAME', 'roadname'))
        self.assertModule(module)

        m = SimpleModule('v.info', flags='c', map='myroads')
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), 'roadname')
        self.assertNotRegexpMatches(decode(m.outputs.stdout), 'ROAD_NAME')

if __name__ == '__main__':
    test()
