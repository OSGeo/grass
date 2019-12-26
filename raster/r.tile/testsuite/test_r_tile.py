"""
Name:        r.tile test
Purpose:    Tests r.tile module and the number of created tiles.

Author:     Shubham Sharma, Google Code-in 2018
Copyright:  (C) 2018 by Shubham Sharma and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRasterTile(TestCase):
    input = 'lakes'
    output_prefix = 'lakes_tile'
    width = '1000'
    height = '1000'
    overlap = '10'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + '-000-000')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + '-000-001')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + '-001-000')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + '-001-001')

        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + 'overlap' + '-000-000')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + 'overlap' + '-000-001')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + 'overlap' + '-001-000')
        cls.runModule('g.remove', type='raster', flags='f', name=cls.output_prefix + 'overlap' + '-001-001')

    def test_raster_tile(self):
        """Testing r.tile runs successfully"""
        self.assertModule('r.tile', input=self.input, output=self.output_prefix, width=self.width, height=self.height)
        # If the above statement executed successful then
        # 4 rasters tiles with following details should exits
        self.assertRasterExists(self.output_prefix+'-000-000', msg="lakes_tile-000-000 does not exits")
        self.assertRasterExists(self.output_prefix+'-000-001', msg="lakes_tile-000-001 does not exits")
        self.assertRasterExists(self.output_prefix+'-001-000', msg="lakes_tile-001-000 does not exits")
        self.assertRasterExists(self.output_prefix+'-001-001', msg="lakes_tile-001-001 does not exits")

    def test_raster_tile_overlap(self):
        """Testing r.tile runs successfully with overlap option"""
        self.assertModule('r.tile', input=self.input, output=self.output_prefix+'overlap', width=self.width, height=self.height, overlap=self.overlap)
        # If the above statement executed successful then
        # 4 rasters tiles with following details should exits
        self.assertRasterExists(self.output_prefix+'overlap'+'-000-000', msg="lakes_tile-000-000 does not exits")
        self.assertRasterExists(self.output_prefix+'overlap'+'-000-001', msg="lakes_tile-000-001 does not exits")
        self.assertRasterExists(self.output_prefix+'overlap'+'-001-000', msg="lakes_tile-001-000 does not exits")
        self.assertRasterExists(self.output_prefix+'overlap'+'-001-001', msg="lakes_tile-001-001 does not exits")




if __name__ == '__main__':
    test()
