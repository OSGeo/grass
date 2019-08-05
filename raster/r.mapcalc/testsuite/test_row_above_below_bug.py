#!/usr/bin/env python3

############################################################################
#
# MODULE:        test_row_above_below_bug.py
# AUTHOR:        Vaclav Petras
# PURPOSE:       Show bug reported in #3067
# COPYRIGHT:     (C) 2016 by Vaclav Petras and the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

# #3067
# r.mapcalc gives wrong result when neighborhood modifier takes a cell
# above first
# https://trac.osgeo.org/grass/ticket/3067

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


INPUT = """\
north: 10
south: 8
east: 20
west: 18
rows: 3
cols: 3
1 0 1
1 1 0
1 1 0
"""

OUTPUT = """\
north: 10
south: 8
east: 20
west: 18
rows: 3
cols: 3
null: *
* * *
2 1 1
* * *
"""

class TestRowAboveAndBelowBug(TestCase):
    # TODO: replace by unified handing of maps
    to_remove = []
    input = 'r_mapcalc_test_input'
    output = 'r_mapcalc_test_output'
    output_ref = 'r_mapcalc_test_output_ref'

    def setUp(self):
        self.use_temp_region()
        self.runModule('r.in.ascii', input='-', stdin_=INPUT,
                       output=self.input, overwrite=True)
        self.to_remove.append(self.input)
        self.runModule('g.region', raster=self.input)
        self.runModule('r.in.ascii', input='-', stdin_=OUTPUT,
                       output=self.output_ref, overwrite=True)
        self.to_remove.append(self.output_ref)

    def tearDown(self):
        self.del_temp_region()
        if 0 and self.to_remove:
            self.runModule('g.remove', flags='f', type='raster',
                           name=','.join(self.to_remove), verbose=True)

    def r_mapcalc_with_test(self, expression):
        """Expects just RHS and inputs as ``{m}`` for format function"""
        expression = expression.format(m=self.input)
        expression = "{0} = {1}".format(self.output, expression)
        self.assertModule('r.mapcalc', expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        ref_univar = dict(null_cells=6, cells=9)
        self.assertRasterFitsUnivar(raster=self.output,
                                    reference=ref_univar, precision=0)
        self.assertRastersNoDifference(actual=self.output,
                                       reference=self.output_ref,
                                       precision=0)  # it's CELL type

    def test_below_above(self):
        self.r_mapcalc_with_test("{m}[1,0] + {m}[-1,0]")

    def test_above_below(self):
        self.r_mapcalc_with_test("{m}[-1,0] + {m}[1,0]")


if __name__ == '__main__':
    test()
