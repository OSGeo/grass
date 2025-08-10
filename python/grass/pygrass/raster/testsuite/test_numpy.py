"""
Created on Thu Jul 30 18:27:22 2015

@author: lucadelu
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows
from numpy.random import default_rng
from grass.pygrass.raster import raster2numpy, numpy2raster, RasterRow


def check_raster(name):
    r = RasterRow(name)
    try:
        r.open(mode="r")
        r.close()
        return True
    except:
        return False


class NumpyTestCase(TestCase):
    name = "RasterRowTestCase_map"

    @classmethod
    def setUpClass(cls):
        """Create test raster map and region"""
        cls.use_temp_region()
        cls.runModule("g.region", n=40, s=0, e=60, w=0, res=1)
        cls.runModule(
            "r.mapcalc",
            expression="%s = float(row() + (10.0 * col()))" % (cls.name),
            overwrite=True,
        )
        cls.numpy_obj = raster2numpy(cls.name)

    @classmethod
    def tearDownClass(cls):
        """Remove the generated vector map, if exists"""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.name)
        cls.del_temp_region()

    def test_type(self):
        self.assertTrue(str(self.numpy_obj.dtype), "float32")

    def test_len(self):
        self.assertTrue(len(self.numpy_obj), 40)
        self.assertTrue(len(self.numpy_obj[0]), 60)

    @xfail_windows
    def test_write(self):
        rng = default_rng()
        numpy2raster(rng.random([40, 60]), "FCELL", self.name, True)
        self.assertTrue(check_raster(self.name))


if __name__ == "__main__":
    test()
