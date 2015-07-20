"""
TEST:      test_addons_modules.py

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test for g.extension individual modules/extensions handling

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

import os


MODULES_OUTPUT = """\
d.frame
d.mon2
g.copyall
g.isis3mt
g.proj.all
r.gdd
r.geomorphon
r.le.patch
r.le.pixel
r.traveltime
r.univar2
v.civil
v.class.ml
v.class.mlpy
v.colors2
v.delaunay3d
v.ellipse
v.in.proj
v.in.redwg
v.neighborhoodmatrix
v.transects
wx.metadata
""".replace('\n', os.linesep)


class TestModulesMetadata(TestCase):

    url = 'file://' + os.path.abspath('data')

    def test_limits(self):
        """Test if results is in expected limits"""
        module = SimpleModule('g.extension', flags='l', svnurl=self.url)
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(stdout, MODULES_OUTPUT)


if __name__ == '__main__':
    test()
