"""Test of raster3d library

@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase

class Raster3dLibraryTest(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.runModule("g.gisenv", set="OVERWRITE=1")
        
    def test_coordinates(self):
        self.assertModule("test.raster3d.lib",  flags="l",  unit="coord")
        self.assertModule("test.raster3d.lib",  unit="coord")

    def test_putget(self):
        self.assertModule("test.raster3d.lib",  unit="putget")
        self.assertModule("test.raster3d.lib",  flags="l",  unit="putget")

    def test_large_tilesizes(self):
        """Test for different tile sizes"""
        self.assertModule("test.raster3d.lib",  unit="large",  depths=91,  rows=89,  cols=87)
        self.assertModule("test.raster3d.lib",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=8)
        self.assertModule("test.raster3d.lib",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=512)
        self.assertModule("test.raster3d.lib",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=1024)
        self.assertModule("test.raster3d.lib",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=32768)

        # Enable zlib compression
        self.assertModule("test.raster3d.lib",  flags="l",  unit="large",  depths=91,  rows=89,  cols=87)
        self.assertModule("test.raster3d.lib",  flags="l",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=8)
        self.assertModule("test.raster3d.lib",  flags="l",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=512)
        self.assertModule("test.raster3d.lib",  flags="l",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=1024)
        self.assertModule("test.raster3d.lib",  flags="l",  unit="large",  depths=91,  rows=89,  cols=87,  tile_size=32768)

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()


