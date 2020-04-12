"""
Created on Mon 17 Feb 2020 02:27:26 PM UTC

@author: Markus Neteler and Māris Nartišs, upon https://github.com/OSGeo/grass/pull/277
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.core import run_command
from grass.gunittest.utils import silent_rmtree


class TestRBlend(TestCase):
    """Test r.blend script"""

    map1 = 'elevation'
    map2 = 'aspect'
    temp1 = 'elev_shade_blend.r'
    temp2 = 'elev_shade_blend.g'
    temp3 = 'elev_shade_blend.b'
    mapsets_to_remove = []
    # mapset with a name is also a valid mathematical expression
    mapset_name = "1234-56-78"
    gisenv = SimpleModule('g.gisenv', get='MAPSET')
    TestCase.runModule(gisenv, expecting_stdout=True)
    old_mapset = gisenv.outputs.stdout.strip()

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        # create a mapset with a name is also a valid mathematical expression
        cls.runModule("g.mapset", flags="c", mapset=cls.mapset_name)
        cls.mapsets_to_remove.append(cls.mapset_name)
        run_command('g.copy', raster=cls.map1 + '@PERMANENT,' + cls.map1)
        cls.runModule('g.region', raster=cls.map1, flags='p')

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data"""
        gisenv = SimpleModule('g.gisenv', get='GISDBASE')
        cls.runModule(gisenv, expecting_stdout=True)
        gisdbase = gisenv.outputs.stdout.strip()
        gisenv = SimpleModule('g.gisenv', get='LOCATION_NAME')
        cls.runModule(gisenv, expecting_stdout=True)
        location = gisenv.outputs.stdout.strip()
        cls.runModule('g.remove', flags='f', type='raster',
                      name=(cls.temp1, cls.temp2, cls.temp3))
        cls.runModule("g.mapset", mapset=cls.old_mapset)
        for mapset_name in cls.mapsets_to_remove:
            mapset_path = os.path.join(gisdbase, location, mapset_name)
            silent_rmtree(mapset_path)

    def test_blend(self):
        """blends test with special mapset name"""

        # should not lead to syntax error, unexpected INTEGER, expecting VARNAME or NAME
        module = SimpleModule('r.blend', first=self.map2, second=self.map1 + '@' + self.mapset_name,
                              output='elev_shade_blend')
        self.assertModule(module)


if __name__ == '__main__':
    test()
