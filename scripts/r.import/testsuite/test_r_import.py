#!/usr/bin/env python3

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.script as gs


class TestRImportRegion(TestCase):

    imported = 'test_r_import_imported'

    @classmethod
    def setUpClass(cls):
        cls.runModule('g.region', raster='elevation')

    def tearDown(cls):
        """Remove imported map after each test method"""
        cls.runModule('g.remove', flags='f', type='raster',
                      name=cls.imported)

    def test_import_estimate(self):
        """Test e flag"""
        self.assertModule('r.import', input='data/data2.asc', output=self.imported,
                          resample='nearest', flags='e')
        self.assertRasterDoesNotExist(name=self.imported)

    def test_import_same_proj_tif(self):
        """Import tif in same proj, default params"""
        self.assertModule('r.import', input='data/data1.tif', output=self.imported,
                          resample='bilinear')
        reference = dict(north=223490, south=223390, east=636820, west=636710,
                         nsres=10, ewres=10, datatype='FCELL')
        self.assertRasterFitsInfo(raster=self.imported, reference=reference, precision=1e-6)

    def test_import_asc_custom_res(self):
        """Import ASC in different projection, with specified resolution"""
        self.assertModule('r.import', input='data/data2.asc', output=self.imported,
                          resample='nearest', resolution='value', resolution_value=30)
        reference = dict(rows=3, cols=4,
                         nsres=30, ewres=30, datatype='CELL')
        self.assertRasterFitsInfo(raster=self.imported, reference=reference, precision=1.1)

    def test_import_asc_region_extent(self):
        """Import ASC in different projection in specified region"""
        self.runModule('g.region', raster='elevation', n=223655, s=223600)
        self.assertModule('r.import', input='data/data2.asc', output=self.imported,
                          resample='nearest', extent='region', resolution='region')
        reference = dict(north=223655, south=223600)
        self.assertRasterFitsInfo(raster=self.imported, reference=reference, precision=1e-6)

    def test_import_use_temp_region(self):
        """Import in specified region with use_temp_region activated"""
        self.runModule('g.region', raster='elevation', n=223660, s=223600)
        gs.use_temp_region()
        self.runModule('g.region', raster='elevation', n=223630, s=223600)
        self.assertModule('r.import', input='data/data2.asc', output=self.imported,
                          resample='nearest', extent='region', resolution='region')
        reference = dict(north=223630, south=223600, nsres=10, ewres=10)
        self.assertRasterFitsInfo(raster=self.imported, reference=reference, precision=1e-6)

        self.runModule('g.region', raster='elevation', s=223390, n=223450)
        self.assertModule('r.import', input='data/data1.tif', output=self.imported,
                          resample='bilinear', extent='region', overwrite=True)
        reference = dict(south=223390, north=223450, nsres=10, ewres=10)
        self.assertRasterFitsInfo(raster=self.imported, reference=reference, precision=1e-6)

        gs.del_temp_region()
        self.assertModule('r.import', input='data/data2.asc', output=self.imported,
                          resample='nearest', extent='region', resolution='region', overwrite=True)
        reference = dict(north=223660, south=223600, nsres=10, ewres=10)
        self.assertRasterFitsInfo(raster=self.imported, reference=reference, precision=1e-6)


if __name__ == '__main__':
    test()
