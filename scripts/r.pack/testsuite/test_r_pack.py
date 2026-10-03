"""
Created on Sun Jun 08 10:15:22 2018

@author: Sanjeet Bhatti
"""

from pathlib import Path
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class TestRPack(TestCase):
    """Test r.pack script"""

    mapName = "aspect"
    outFile = "aspect.pack"

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.mapName, flags="p")

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region. Delete output file"""
        cls.del_temp_region()

        Path(cls.outFile).unlink(missing_ok=True)

    def test_r_pack(self):
        """Create a pack file test"""
        module = SimpleModule("r.pack", input=self.mapName, output=self.outFile)
        self.assertModule(module)

        self.assertFileExists(filename=self.outFile)


if __name__ == "__main__":
    test()
