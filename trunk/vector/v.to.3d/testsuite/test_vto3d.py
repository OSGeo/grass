from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestVTo3D(TestCase):

    contours2d = 'test_vto3d_contours2d'
    contours3d = 'test_vto3d_contours3d'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster='elevation')

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(cls):
        """Remove contours map after each test method"""
        cls.runModule('g.remove', flags='f', type='vector',
                      name=[cls.contours2d, cls.contours3d])

    def test_contours(self):
        """Test if results is in expected limits"""
        self.runModule('r.contour', input='elevation', output=self.contours3d, step=5)
        self.runModule('v.db.addcolumn', map=self.contours3d, columns="z double precision")

        self.assertModule('v.to.3d', input=self.contours3d, output=self.contours2d,
                          column='z', flags='r')
        is3d = dict(map3d=0)
        self.assertVectorFitsTopoInfo(vector=self.contours2d, reference=is3d)
        missing = dict(nmissing=0, nnull=0)
        self.assertVectorFitsUnivar(map=self.contours2d, column='z', reference=missing)


if __name__ == '__main__':
    test()
