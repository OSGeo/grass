# -*- coding: utf-8 -*-
from grass.exceptions import OpenError
from grass.gunittest import TestCase, test
from unittest import skip

from grass.pygrass.raster import RasterRow


class RasterRowTestCate(TestCase):

    @classmethod
    def setUpClass(cls):
        """Create a not empty table instance"""
        from grass.pygrass.modules.shortcuts import general as g

        cls.name = 'elevation'
        cls.tmp = 'tmp' + cls.name
        g.copy(rast=[cls.name, cls.tmp], overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exist"""
        from grass.pygrass.modules.shortcuts import general as g
        g.remove(type='rast', pattern=cls.tmp, flags='f')

    def test_type(self):
        r = RasterRow(self.name)
        r.open(mode='r')
        self.assertTrue(r.mtype,'FCELL')
        r.close()

    def test_isopen(self):
        r = RasterRow(self.name)
        self.assertFalse(r.is_open())
        r.open(mode='r')
        self.assertTrue(r.is_open())
        r.close()
        self.assertFalse(r.is_open())

    def test_name(self):
        r = RasterRow(self.name)
        r.open(mode='r')
        self.assertEqual(r.name, self.name)
        fullname = "{name}@{mapset}".format(name=r.name, mapset=r.mapset)
        self.assertEqual(r.fullname(), fullname)
        r.close()

    def test_exist(self):
        notexist = RasterRow(self.name + 'notexist')
        self.assertFalse(notexist.exist())
        exist = RasterRow(self.name)
        self.assertTrue(exist.exist())

    def test_open_r(self):
        notexist = RasterRow(self.tmp + 'notexist')
        with self.assertRaises(OpenError):
            # raster does not exist
            notexist.open(mode='r')
        r = RasterRow(self.name)
        r.open(mode='r', mtype='DCELL')
        # ignore the mtype if is open in read mode
        self.assertEqual(r.mtype, 'FCELL')
        r.close()

    def test_open_w(self):
        r = RasterRow(self.tmp)
        with self.assertRaises(OpenError):
            # raster type is not defined!
            r.open(mode='w')
        with self.assertRaises(OpenError):
            # raster already exist
            r.open(mode='w', mtype='DCELL')
        # open in write mode and overwrite
        r.open(mode='w', mtype='DCELL', overwrite=True)
        self.assertTrue(r.mtype, 'DCELL')


if __name__ == '__main__':
    test()
