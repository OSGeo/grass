#!/usr/bin/env python3

############################################################################
#
# MODULE:        test_nmedian_bug.py
# AUTHOR:        Maris Nartiss, based on Vaclav Petras test_row_above_below_bug.py
# PURPOSE:       Show bug reported in #3296
# COPYRIGHT:     (C) 2017 by Maris Nartiss and the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

# #3296
# r.mapcalc calculates wrong median value if one of two input maps has a null value
# Also depends on correct functioning of row() and col() mapcalc variables
# https://trac.osgeo.org/grass/ticket/3296

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


OUTPUT = """\
north: 3
south: 0
west: 0
east: 3
rows: 3
cols: 3
null: *
1.0 1.0 1.0
4.0 4.0 4.0
9.0 9.0 9.0
"""

OUTPUT_CELL = """\
north: 3
south: 0
west: 0
east: 3
rows: 3
cols: 3
null: *
1 1 1
4 4 4
9 9 9
"""


class TestNmedianBug(TestCase):
    to_remove = []
    input = 'r_mapcalc_test_pattern'
    output = 'r_mapcalc_test_output'
    output_ref = 'r_mapcalc_test_output_ref'
    output_cell = 'r_mapcalc_test_output_cell'

    def setUp(self):
        expression = "{o}=row()*col()".format(o=self.input)
        self.use_temp_region()
        self.runModule('g.region', n=3, s=0, e=3, w=0, res=1)
        self.runModule('r.mapcalc', expression=expression, overwrite=True)
        self.to_remove.append(self.input)
        self.runModule('r.in.ascii', input='-', stdin_=OUTPUT,
                       output=self.output_ref, overwrite=True)
        self.to_remove.append(self.output_ref)
        self.runModule('r.in.ascii', input='-', stdin_=OUTPUT_CELL,
                       output=self.output_cell, overwrite=True)
        self.to_remove.append(self.output_cell)

    def tearDown(self):
        self.del_temp_region()
        if 0 and self.to_remove:
            self.runModule('g.remove', flags='f', type='raster',
                           name=','.join(self.to_remove), verbose=True)

    def test_cell(self):
        expression = "{o}=nmedian(({i}[0,-1] - {i})^2,({i}[0,1] - {i})^2)".format(o=self.output, i=self.input)
        self.assertModule('r.mapcalc', expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(actual=self.output,
            reference=self.output_cell, precision=0)

    def test_fcell(self):
        expression = "{o}=nmedian(float(({i}[0,-1] - {i})^2), float(({i}[0,1] - {i})^2))".format(o=self.output, i=self.input)
        self.assertModule('r.mapcalc', expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(actual=self.output,
            reference=self.output_ref, precision=0)
    
    def test_dcell(self):
        expression = "{o}=nmedian(double(({i}[0,-1] - {i})^2), double(({i}[0,1] - {i})^2))".format(o=self.output, i=self.input)
        self.assertModule('r.mapcalc', expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(actual=self.output,
            reference=self.output_ref, precision=0)


if __name__ == '__main__':
    test()
