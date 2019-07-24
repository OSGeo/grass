"""
TEST:      test_addons_toolboxes.py

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test for g.extension toolboxes handling

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

import os

FULL_TOOLBOXES_OUTPUT = u"""\
Hydrology (HY)
* r.stream.basins
* r.stream.channel
* r.stream.distance
* r.stream.order
* r.stream.segment
* r.stream.slope
* r.stream.snap
* r.stream.stats
mcda (MC)
* r.mcda.ahp
* r.mcda.roughset
* r.mcda.input
* r.mcda.output
"""


class TestToolboxesMetadata(TestCase):

    url = 'file://' + os.path.abspath('data')

    def test_listing(self):
        """List toolboxes and their content"""
        module = SimpleModule('g.extension', flags='lt', url=self.url)
        self.assertModule(module)
        stdout = module.outputs.stdout
        self.assertMultiLineEqual(stdout, FULL_TOOLBOXES_OUTPUT)


if __name__ == '__main__':
    test()
