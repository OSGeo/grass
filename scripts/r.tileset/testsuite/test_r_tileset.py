"""
Created on Sun Jun 08 19:42:32 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.utils import decode

import os

output = """\
-78.77462049|35.6875073|-78.60830318|35.74855834|1506|678
-78.77462049|35.74855834|-78.60830318|35.80960938|1506|678
""".replace('\n', os.linesep)


class TestRTileset(TestCase):
    """Test r.tileset script"""

    mapName = 'elevation'

    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.mapName, flags='p')

    @classmethod
    def tearDownClass(cls):
        """!Remove the temporary region"""
        cls.del_temp_region()

    def test_tiling(self):
        """Produce tiling test"""
        module = SimpleModule('r.tileset', sourceproj='+init=epsg:4326',
                              maxrows=1024, maxcols=2048)
        self.assertModule(module)

        self.assertMultiLineEqual(decode(module.outputs.stdout), output)

if __name__ == '__main__':
    test()
