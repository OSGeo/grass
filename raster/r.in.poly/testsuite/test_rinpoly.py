import os
import tempfile
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command


input1 = \
b"""
A
634308.630394 223320.356473
640640.712946 223092.401501
641248.592871 217748.123827
= 10.01 label1
A
639576.923077 222256.566604
639045.028143 216329.737336
637702.626642 224662.757974
= -8
L
633523.452158 222231.238274
642565.666041 221218.105066
641957.786116 222585.834897
= 3 label2
"""


class TestRInPoly(TestCase):

    rinpoly = 'test_rinpoly'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster='elevation')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def setUp(self):
        self.tmpFile = tempfile.NamedTemporaryFile(delete=False)

    def tearDown(self):
        """Remove rinpoly map after each test method"""
        self.runModule('g.remove', flags='f', type='raster',
                       name=self.rinpoly)
        os.unlink(self.tmpFile.name)

    def testTypeCell(self):
        """Test type of resulting map"""
        self.tmpFile.write(input1)
        self.tmpFile.close()
        self.assertModule('r.in.poly', input=self.tmpFile.name, output=self.rinpoly, type='CELL')
        minmax = 'min=-8\nmax=10\ndatatype=CELL'
        self.assertRasterFitsInfo(raster=self.rinpoly, reference=minmax)

    def testTypeFCell(self):
        """Test type of resulting map"""
        self.tmpFile.write(input1)
        self.tmpFile.close()
        self.assertModule('r.in.poly', input=self.tmpFile.name, output=self.rinpoly, type='FCELL')
        minmax = 'min=-8\nmax=10.01\ndatatype=FCELL'
        self.assertRasterFitsInfo(raster=self.rinpoly, reference=minmax, precision=1e-8)

    def testTypeDCell(self):
        """Test type of resulting map"""
        self.tmpFile.write(input1)
        self.tmpFile.close()
        self.assertModule('r.in.poly', input=self.tmpFile.name, output=self.rinpoly, type='DCELL')
        minmax = 'min=-8\nmax=10.01\ndatatype=DCELL'
        self.assertRasterFitsInfo(raster=self.rinpoly, reference=minmax, precision=1e-8)

    def testTypeCellNull(self):
        """Test type of resulting map"""
        self.tmpFile.write(input1)
        self.tmpFile.close()
        self.assertModule('r.in.poly', input=self.tmpFile.name, output=self.rinpoly, type='CELL',
                          null=-8)
        minmax = 'min=3\nmax=10\ndatatype=CELL'
        self.assertRasterFitsInfo(raster=self.rinpoly, reference=minmax, precision=1e-8)

    def testTypeDCellNull(self):
        """Test type of resulting map"""
        self.tmpFile.write(input1)
        self.tmpFile.close()
        self.assertModule('r.in.poly', input=self.tmpFile.name, output=self.rinpoly, type='DCELL',
                          null=-8)
        minmax = 'min=3\nmax=10.01\ndatatype=DCELL'
        self.assertRasterFitsInfo(raster=self.rinpoly, reference=minmax, precision=1e-8)

    def testTypeDCellNull2(self):
        """Test type of resulting map"""
        self.tmpFile.write(input1)
        self.tmpFile.close()
        self.assertModule('r.in.poly', input=self.tmpFile.name, output=self.rinpoly, type='DCELL',
                          null=0)
        minmax = 'min=-8\nmax=10.01\ndatatype=DCELL'
        self.assertRasterFitsInfo(raster=self.rinpoly, reference=minmax, precision=1e-8)

    def testLabels(self):
        """Test type of resulting map"""
        self.tmpFile.write(input1)
        self.tmpFile.close()
        self.assertModule('r.in.poly', input=self.tmpFile.name, output=self.rinpoly, type='DCELL')
        category = read_command('r.category', map=self.rinpoly, values=[-8, 3, 10.01]).strip()
        self.assertEqual(first="-8\t{newline}3\tlabel2{newline}10.01".format(newline=os.linesep),
                         second=category, msg="Labels do not match")


if __name__ == '__main__':
    test()
