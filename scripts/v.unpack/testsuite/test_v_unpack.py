"""
Created on Sun Jun 08 12:50:45 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

import os


class TestVUnpack(TestCase):
    """Test v.unpack script"""

    mapName = 'roadsmajor'
    packFile = 'roadsmajor.pack'

    @classmethod
    def setUpClass(cls):
        """Run v.pack to create packfile."""
        cls.runModule('v.pack', input=cls.mapName, output=cls.packFile)

    @classmethod
    def tearDownClass(cls):
        """Remove pack file created region"""
        cls.runModule('g.remove', type='vector', name=cls.mapName, flags='f')
        if (os.path.isfile(cls.packFile)):
            os.remove(cls.packFile)

    def test_v_pack(self):
        """Unpack file test"""
        module = SimpleModule('v.unpack', input=self.packFile, overwrite=True)
        self.assertModule(module)

if __name__ == '__main__':
    test()
