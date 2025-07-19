#!/usr/bin/env python3

############################################################################
#
# MODULE:        test_median_mode.py
# AUTHOR:        Anna Petrasova, based on Vaclav Petras test_row_above_below_bug.py
# PURPOSE:       Test median, mode in r.mapcalc
# COPYRIGHT:     (C) 2025 by Anna Petrasova and the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################


from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestMedianMode(TestCase):
    to_remove = []
    input = "r_mapcalc_test_input"
    input_null = "r_mapcalc_test_input_null"
    output = "r_mapcalc_test_output"

    @classmethod
    def setUpClass(cls):
        expression = "{o}=row()*col()".format(o=cls.input)
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)
        cls.runModule("r.mapcalc", expression=expression, overwrite=True)
        cls.to_remove.append(cls.input)
        expression = "{o}=null()".format(o=cls.input_null)
        cls.runModule("r.mapcalc", expression=expression, overwrite=True)
        cls.to_remove.append(cls.input_null)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=cls.to_remove,
        )

    def test_median_cell(self):
        expression = "{o} = median({i}, {i} + 1, {i} - 1, {i})".format(
            o=self.output, i=self.input
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_median_dcell(self):
        expression = "{o} = median(double({i}), double({i} + 1), double({i} - 1), double({i}))".format(
            o=self.output, i=self.input
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_median_cell_many_numbers(self):
        expression = "{o} = median({numbers})".format(
            o=self.output, numbers=",".join([str(n) for n in list(range(1, 50))])
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 25, "range": 0}, precision=1e-6
        )

    def test_median_cell_many_numbers_null(self):
        expression = "{o} = median({numbers}, null())".format(
            o=self.output, numbers=",".join([str(n) for n in list(range(1, 50))])
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(self.output, {"n": 0}, precision=1e-6)

        expression = "{o} = nmedian({numbers}, null())".format(
            o=self.output, numbers=",".join([str(n) for n in list(range(1, 50))])
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 25, "range": 0}, precision=1e-6
        )

    def test_mode_cell(self):
        expression = "{o} = mode({i}, {i} + 1, {i} - 1, {i})".format(
            o=self.output, i=self.input
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_mode_dcell(self):
        expression = "{o} = mode(double({i}), double({i} + 1), double({i} - 1), double({i}))".format(
            o=self.output, i=self.input
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_mode_cell_many_numbers(self):
        expression = "{o} = mode(1, {numbers})".format(
            o=self.output, numbers=",".join([str(n) for n in list(range(1, 50))])
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_mode_cell_many_numbers_null(self):
        expression = "{o} = mode(1, {numbers}, null())".format(
            o=self.output, numbers=",".join([str(n) for n in list(range(1, 50))])
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(self.output, {"n": 0}, precision=1e-6)

        expression = "{o} = nmode(1, {numbers}, null())".format(
            o=self.output, numbers=",".join([str(n) for n in list(range(1, 50))])
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )


if __name__ == "__main__":
    test()
