"""
Created on Sun Jun 08 23:58:10 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command


class TestVDissolve(TestCase):
    """Test v.dissolve script"""

    mapName = 'mysoils_general'
    outputName = 'mysoils_general_families'

    @classmethod
    def setUpClass(cls):
        """Copy vect"""
        run_command('g.copy', vector='soils_general,mysoils_general')

    @classmethod
    def tearDownClass(cls):
        """Remove vector"""
        run_command('g.remove', flags='f', type='vector',
                    name=(cls.mapName, cls.outputName))

    def test_dissolve(self):
        """dissolve test"""
        module = SimpleModule('v.dissolve', input=self.mapName,
                              output=self.outputName, column='GSL_NAME')
        self.assertModule(module)

        self.assertVectorExists(self.outputName)

if __name__ == '__main__':
    test()
