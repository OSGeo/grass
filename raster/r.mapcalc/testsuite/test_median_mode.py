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
        expression = f"{cls.input} = row()*col()"
        cls.use_temp_region()
        cls.runModule("g.region", n=3, s=0, e=3, w=0, res=1)
        cls.runModule("r.mapcalc", expression=expression, overwrite=True)
        cls.to_remove.append(cls.input)
        expression = f"{cls.input_null} = null()"
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
        expression = f"{self.output} = median({self.input}, {self.input} + 1, {self.input} - 1, {self.input})"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_median_dcell(self):
        expression = f"{self.output} = median(double({self.input}), double({self.input} + 1), double({self.input} - 1), double({self.input}))"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_median_cell_many_numbers(self):
        expression = (
            f"{self.output} = median({','.join([str(n) for n in list(range(1, 50))])})"
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 25, "range": 0}, precision=1e-6
        )

    def test_median_cell_many_numbers_null(self):
        expression = f"{self.output} = median({','.join([str(n) for n in list(range(1, 50))])}, null())"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(self.output, {"n": 0}, precision=1e-6)

        expression = f"{self.output} = nmedian({','.join([str(n) for n in list(range(1, 50))])}, null())"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 25, "range": 0}, precision=1e-6
        )

    def test_mode_cell(self):
        expression = f"{self.output} = mode({self.input}, {self.input} + 1, {self.input} - 1, {self.input})"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_mode_dcell(self):
        expression = f"{self.output} = mode(double({self.input}), double({self.input} + 1), double({self.input} - 1), double({self.input}))"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRastersNoDifference(
            actual=self.output, reference=self.input, precision=0
        )

    def test_mode_cell_many_numbers(self):
        expression = (
            f"{self.output} = mode(1, {','.join([str(n) for n in list(range(1, 50))])})"
        )
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )

    def test_mode_cell_many_numbers_null(self):
        expression = f"{self.output} = mode(1, {','.join([str(n) for n in list(range(1, 50))])}, null())"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.to_remove.append(self.output)
        self.assertRasterFitsUnivar(self.output, {"n": 0}, precision=1e-6)

        expression = f"{self.output} = nmode(1, {','.join([str(n) for n in list(range(1, 50))])}, null())"
        self.assertModule("r.mapcalc", expression=expression, overwrite=True)
        self.assertRasterExists(self.output)
        self.assertRasterFitsUnivar(
            self.output, {"mean": 1, "range": 0}, precision=1e-6
        )


if __name__ == "__main__":
    test()
