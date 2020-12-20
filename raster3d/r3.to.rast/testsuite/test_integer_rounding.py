#!/usr/bin/env python3

############################################################################
#
# MODULE:        test_integer_rounding
# AUTHOR:        Vaclav Petras
# PURPOSE:       Fast test of rounding integers based on a small example
# COPYRIGHT:     (C) 2016 by Vaclav Petras and the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script import list_strings

# generated by
# g.region n=12 s=9 e=21 w=18 t=8 b=4 res=1 res3=1 -p3
# r3.mapcalc "x = rand(0,5.)" seed=100 && r3.out.ascii x prec=2
INPUT = """\
version: grass7
order: nsbt
north: 12.000000
south: 9.000000
east: 21.000000
west: 18.000000
top: 8.000000
bottom: 4.000000
rows: 3
cols: 3
levels: 4
1.26 1.04 4.70
2.11 1.98 1.91
1.16 2.68 2.67
4.31 4.81 1.58
3.68 3.27 0.25
2.27 4.03 3.28
4.06 4.15 0.40
1.02 3.40 3.26
4.88 0.43 1.02
1.16 0.07 0.78
0.07 3.52 0.45
2.86 4.79 1.43
"""

# created from the above and template from
# r.mapcalc "x = rand(0,10)" seed=100 && r.out.ascii x prec=0
OUTPUTS = [
"""\
north: 12
south: 9
east: 21
west: 18
rows: 3
cols: 3
1 1 5
2 2 2
1 3 3
""",
"""\
north: 12
south: 9
east: 21
west: 18
rows: 3
cols: 3
4 5 2
4 3 0
2 4 3
""",
"""\
north: 12
south: 9
east: 21
west: 18
rows: 3
cols: 3
4 4 0
1 3 3
5 0 1
""",
"""\
north: 12
south: 9
east: 21
west: 18
rows: 3
cols: 3
1 0 1
0 4 0
3 5 1
""",
]


class TestR3ToRastIntegerRounding(TestCase):
    # TODO: replace by unified handing of maps
    # mixing class and object attributes
    to_remove_3d = []
    to_remove_2d = []
    rast3d = 'r3_to_rast_test_int_round'
    rast2d = 'r3_to_rast_test_int_round'
    rast2d_ref = 'r3_to_rast_test_int_round_ref'
    rast2d_refs = []

    def setUp(self):
        self.use_temp_region()
        self.runModule('r3.in.ascii', input='-', stdin_=INPUT,
                       output=self.rast3d)
        self.to_remove_3d.append(self.rast3d)
        self.runModule('g.region', raster_3d=self.rast3d)

        for i, data in enumerate(OUTPUTS):
            rast = "%s_%d" % (self.rast2d_ref, i)
            self.runModule('r.in.ascii', input='-', stdin_=data,
                           output=rast, type='CELL')
            self.to_remove_2d.append(rast)
            self.rast2d_refs.append(rast)

    def tearDown(self):
        if self.to_remove_3d:
            self.runModule('g.remove', flags='f', type='raster_3d',
                           name=','.join(self.to_remove_3d), verbose=True)
        if self.to_remove_2d:
            self.runModule('g.remove', flags='f', type='raster',
                           name=','.join(self.to_remove_2d), verbose=True)
        self.del_temp_region()

    def test_rounding(self):
        self.assertModule('r3.to.rast', input=self.rast3d,
                          output=self.rast2d, type='CELL', add=0.5)
        rasts = list_strings('raster', subproject=".",
                             pattern="%s_*" % self.rast2d,
                             exclude="%s_*" % self.rast2d_ref)
        self.assertEquals(len(rasts), 4,
                          msg="Wrong number of 2D rasters present"
                              " in the subproject")
        ref_info = dict(cells=9)
        ref_univar = dict(cells=9, null_cells=0)
        for rast in rasts:
            self.assertRasterExists(rast)
            # the following doesn't make much sense because we just listed them
            self.to_remove_2d.append(rast)
            self.assertRasterFitsInfo(raster=rast, reference=ref_info,
                                      precision=0)
            self.assertRasterFitsUnivar(raster=rast, reference=ref_univar,
                                        precision=0)

        # check the actual values
        for rast_ref, rast in zip(self.rast2d_refs, rasts):
            self.assertRastersNoDifference(actual=rast,
                                           reference=rast_ref,
                                           precision=0.1)


if __name__ == '__main__':
    test()
