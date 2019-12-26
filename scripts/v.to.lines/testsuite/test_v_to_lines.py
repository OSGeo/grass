"""
Created on Sun Jun 09 12:28:03 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestVDToLines(TestCase):
    """Test v.to.lines script"""

    inputMap = 'boundary_municp'
    outputMap = 'boundary_municp_lines'

    inputMap2 = 'geodetic_pts'
    outputMap2 = 'geodetic_pts_lines'

    @classmethod
    def tearDownClass(cls):
        """Remove created vector"""
        cls.runModule('g.remove', type='vector', name=(cls.outputMap,
                      cls.outputMap2), flags='f')

    def test_area_to_line_check(self):
        """Area to line conversion test"""
        module = SimpleModule('v.to.lines', input=self.inputMap,
                              output=self.outputMap)
        self.assertModule(module)

        self.assertVectorExists(self.outputMap)

    def test_point_to_line_check(self):
        """Point to line conversion test"""
        module = SimpleModule('v.to.lines', input=self.inputMap2,
                              output=self.outputMap2, overwrite=True)
        self.assertModule(module)

        self.assertVectorExists(self.outputMap2)

if __name__ == '__main__':
    test()
