"""
Created on Sun Jun 09 01:52:34 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command


class TestVDbDropRow(TestCase):
    """Test v.db.droprow script"""

    mapName = "elevation"
    inputMap = "rand5k_elev"
    outputMap = "rand5k_elev_filt"
    values = "min=100.013\nmax=149.102"

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

        cls.runModule('g.remove', flags='f', type='vector',
                      name=(cls.inputMap, cls.outputMap))

    def test_drop_row_check(self):
        """Drop vector object from a vector map"""
        run_command("v.mkgrid", map=self.inputMap, grid=[10, 10], type="point")
        run_command(
            "v.db.addcolumn", map=self.inputMap, column="elevation double precision"
        )
        run_command(
            "v.what.rast", map=self.inputMap, raster=self.mapName, column=self.mapName
        )

        module = SimpleModule(
            "v.db.droprow",
            input=self.inputMap,
            output=self.outputMap,
            where="elevation < 100",
        )
        self.assertModule(module)

        self.assertVectorFitsUnivar(
            map=self.outputMap,
            column="elevation",
            reference=self.values,
            precision=0.001,
        )

if __name__ == '__main__':
    test()
