"""
TEST:      r.patch

AUTHOR(S): Vaclav Petras

PURPOSE:   Test r.patch using artificial and small data

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module


cell_1 = """\
north: 20
south: 10
east: 25
west: 15
rows: 4
cols: 4
505 501 500 520
506 501 550 520
504 401 400 520
502 520 540 520
"""

cell_2 = """\
north: 30
south: 20
east: 25
west: 15
rows: 4
cols: 4
505 505 500 520
406 409 550 520
304 405 400 560
302 520 540 520
"""

cell_patched_ref = """\
north: 30
south: 10
east: 25
west: 15
rows: 8
cols: 4
505 505 500 520
406 409 550 520
304 405 400 560
302 520 540 520
505 501 500 520
506 501 550 520
504 401 400 520
502 520 540 520
"""

# the following comes from the manual

cell_overlap_a = """\
north: 35
south: 20
east: 25
west: 15
rows: 6
cols: 8
1 1 1 0 2 2 0 0
1 1 0 2 2 2 0 0
3 3 3 3 2 2 0 0
3 3 3 3 0 0 0 0
3 3 3 0 0 0 0 0
0 0 0 0 0 0 0 0
"""

cell_overlap_b = """\
north: 35
south: 20
east: 25
west: 15
rows: 6
cols: 8
0 0 1 1 0 0 0 0
0 0 1 1 0 0 0 0
0 0 0 0 0 0 0 0
4 4 4 4 4 4 4 4
4 4 4 4 4 4 4 4
4 4 4 4 4 4 4 4
"""

cell_overlap_ab = """\
north: 35
south: 20
east: 25
west: 15
rows: 6
cols: 8
1 1 1 1 2 2 0 0
1 1 1 2 2 2 0 0
3 3 3 3 2 2 0 0
3 3 3 3 4 4 4 4
3 3 3 4 4 4 4 4
4 4 4 4 4 4 4 4
"""

cell_overlap_ba = """\
north: 35
south: 20
east: 25
west: 15
rows: 6
cols: 8
1 1 1 1 2 2 0 0
1 1 1 1 2 2 0 0
3 3 3 3 2 2 0 0
4 4 4 4 4 4 4 4
4 4 4 4 4 4 4 4
4 4 4 4 4 4 4 4
"""


class TestSmallDataNoOverlap(TestCase):
    # TODO: replace by unified handing of maps
    to_remove = []
    cell_1 = "rpatch_small_test_cell_1"
    cell_2 = "rpatch_small_test_cell_2"
    cell_patched = "rpatch_small_test_cell_patched"
    cell_patched_threaded = "rpatch_small_test_cell_patched_threaded"
    cell_patched_ref = "rpatch_small_test_cell_patched_ref"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # 10 / 4 == 2.5 (size of raster / number of cells)
        cls.runModule("g.region", n=30, s=10, e=25, w=15, res=2.5)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
                verbose=True,
            )

    def test_patching_cell(self):
        """Test patching two neighboring CELL raster maps"""
        self.runModule("r.in.ascii", input="-", stdin=cell_1, output=self.cell_1)
        self.to_remove.append(self.cell_1)
        self.runModule("r.in.ascii", input="-", stdin=cell_2, output=self.cell_2)
        self.to_remove.append(self.cell_2)

        self.assertModule(
            "r.patch", input=(self.cell_1, self.cell_2), output=self.cell_patched
        )
        self.assertModule(
            "r.patch",
            input=(self.cell_1, self.cell_2),
            nprocs=8,
            output=self.cell_patched_threaded,
        )
        self.to_remove.append(self.cell_patched)
        self.to_remove.append(self.cell_patched_threaded)
        self.runModule(
            "r.in.ascii",
            input="-",
            stdin=cell_patched_ref,
            output=self.cell_patched_ref,
        )
        self.to_remove.append(self.cell_patched_ref)
        self.assertRastersNoDifference(
            self.cell_patched, self.cell_patched_ref, precision=0
        )
        self.assertRastersNoDifference(
            self.cell_patched_threaded, self.cell_patched_ref, precision=0
        )


class TestSmallDataOverlap(TestCase):
    # TODO: replace by unified handing of maps
    to_remove = []
    cell_a = "rpatch_small_test_cell_a"
    cell_b = "rpatch_small_test_cell_b"
    cell_ab = "rpatch_small_test_cell_ab_reference"
    cell_ba = "rpatch_small_test_cell_ba_reference"
    cell_ab_result = "rpatch_small_test_cell_ab_result"
    cell_ab_result_threaded = "rpatch_small_test_cell_ab_result_threaded"
    cell_ba_result = "rpatch_small_test_cell_ba_result"
    cell_ba_result_threaded = "rpatch_small_test_cell_ba_result_threaded"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # 15 / 6 == 2.5 ((n-s) / number of cells)
        # 10 / 8 == 1.25 ((e-w) / number of cells)
        cls.runModule("g.region", n=35, s=20, e=25, w=15, nsres=2.5, ewres=1.25)
        cls.runModule("r.in.ascii", input="-", stdin=cell_overlap_a, output=cls.cell_a)
        cls.to_remove.append(cls.cell_a)
        cls.runModule("r.in.ascii", input="-", stdin=cell_overlap_b, output=cls.cell_b)
        cls.to_remove.append(cls.cell_b)
        cls.runModule(
            "r.in.ascii", input="-", stdin=cell_overlap_ab, output=cls.cell_ab
        )
        cls.to_remove.append(cls.cell_ab)
        cls.runModule(
            "r.in.ascii", input="-", stdin=cell_overlap_ba, output=cls.cell_ba
        )
        cls.to_remove.append(cls.cell_ba)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
                verbose=True,
            )

    def test_patch_oder_ab_cell(self):
        """Test patching two overlapping CELL raster maps (watching order)"""
        self.assertModule(
            "r.patch",
            input=(self.cell_a, self.cell_b),
            output=self.cell_ab_result,
            flags="z",
        )
        self.assertModule(
            "r.patch",
            input=(self.cell_a, self.cell_b),
            output=self.cell_ab_result_threaded,
            flags="z",
            nprocs=8,
        )
        self.assertRasterExists(self.cell_ab_result)
        self.assertRasterExists(self.cell_ab_result_threaded)
        self.to_remove.append(self.cell_ab_result)
        self.to_remove.append(self.cell_ab_result_threaded)
        self.assertRastersNoDifference(self.cell_ab_result, self.cell_ab, precision=0)
        self.assertRastersNoDifference(
            self.cell_ab_result_threaded, self.cell_ab, precision=0
        )

    def test_patch_oder_ba_cell(self):
        """Test patching two overlapping CELL raster maps (watching order)"""
        self.assertModule(
            "r.patch",
            input=(self.cell_b, self.cell_a),
            output=self.cell_ba_result,
            flags="z",
        )
        self.assertModule(
            "r.patch",
            input=(self.cell_b, self.cell_a),
            flags="z",
            output=self.cell_ba_result_threaded,
            nprocs=8,
        )
        self.assertRasterExists(self.cell_ba_result)
        self.assertRasterExists(self.cell_ba_result_threaded)
        self.to_remove.append(self.cell_ba_result)
        self.to_remove.append(self.cell_ba_result_threaded)
        self.assertRastersNoDifference(self.cell_ba_result, self.cell_ba, precision=0)
        self.assertRastersNoDifference(
            self.cell_ba_result_threaded, self.cell_ba, precision=0
        )


class TestCELLColorCategory(TestCase):
    to_remove = []
    cell_a = "rpatch_small_test_cell_a"
    cell_b = "rpatch_small_test_cell_b"
    cell_ab_result = "rpatch_small_test_cell_ab_result"
    cell_ab_result_threaded = "rpatch_small_test_cell_ab_result_threaded"
    cell_ab_s_result = "rpatch_small_test_cell_ab_s_result"
    cell_ab_s_result_threaded = "rpatch_small_test_cell_ab_s_result_threaded"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # 15 / 6 == 2.5 ((n-s) / number of cells)
        # 10 / 8 == 1.25 ((e-w) / number of cells)
        cls.runModule("g.region", n=35, s=20, e=25, w=15, nsres=2.5, ewres=1.25)
        cls.runModule(
            "r.in.ascii",
            input="-",
            type="CELL",
            stdin=cell_overlap_a,
            output=cls.cell_a,
        )
        cls.to_remove.append(cls.cell_a)
        cls.runModule(
            "r.in.ascii",
            input="-",
            type="CELL",
            stdin=cell_overlap_b,
            output=cls.cell_b,
        )
        cls.to_remove.append(cls.cell_b)
        cls.runModule(
            "r.category",
            map=cls.cell_a,
            rules="-",
            stdin="0:val0\n1:val1\n2:val2\n3:val3",
            separator=":",
        )
        cls.runModule(
            "r.category",
            map=cls.cell_b,
            rules="-",
            stdin="0:val0\n1:val1\n4:val4",
            separator=":",
        )
        cls.runModule(
            "r.colors",
            map=cls.cell_a,
            rules="-",
            stdin="0 0:0:0\n1 1:1:1\n2 2:2:2\n3 3:3:3",
        )
        cls.runModule(
            "r.colors", map=cls.cell_b, rules="-", stdin="0 0:0:0\n1 1:1:1\n4 4:4:4"
        )

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove",
                flags="f",
                type="raster",
                name=",".join(cls.to_remove),
                verbose=True,
            )

    def test_patch_categories_and_color(self):
        """Test patching two overlapping CELL raster maps (watching order)"""
        self.assertModule(
            "r.patch",
            input=(self.cell_a, self.cell_b),
            output=self.cell_ab_result,
            flags="z",
        )
        self.assertModule(
            "r.patch",
            input=(self.cell_a, self.cell_b),
            output=self.cell_ab_s_result,
            flags="zs",
        )
        self.assertModule(
            "r.patch",
            input=(self.cell_a, self.cell_b),
            output=self.cell_ab_result_threaded,
            flags="z",
            nprocs=8,
        )
        self.assertModule(
            "r.patch",
            input=(self.cell_a, self.cell_b),
            output=self.cell_ab_s_result_threaded,
            flags="zs",
            nprocs=8,
        )
        self.assertRasterExists(self.cell_ab_result)
        self.assertRasterExists(self.cell_ab_s_result)
        self.assertRasterExists(self.cell_ab_result_threaded)
        self.assertRasterExists(self.cell_ab_s_result_threaded)
        self.to_remove.append(self.cell_ab_result)
        self.to_remove.append(self.cell_ab_s_result)
        self.to_remove.append(self.cell_ab_result_threaded)
        self.to_remove.append(self.cell_ab_s_result_threaded)
        categories = call_module(
            "r.category", map=self.cell_ab_result, separator="comma"
        ).strip()
        self.assertMultiLineEqual(categories, "0,val0\n1,val1\n2,val2\n3,val3\n4,val4")
        categories = call_module(
            "r.category", map=self.cell_ab_result_threaded, separator="comma"
        ).strip()
        self.assertMultiLineEqual(categories, "0,val0\n1,val1\n2,val2\n3,val3\n4,val4")
        categories = call_module(
            "r.category", map=self.cell_ab_s_result, separator="comma"
        ).strip()
        self.assertMultiLineEqual(categories, "0,\n1,\n2,\n3,\n4,")
        categories = call_module(
            "r.category", map=self.cell_ab_s_result_threaded, separator="comma"
        ).strip()
        self.assertMultiLineEqual(categories, "0,\n1,\n2,\n3,\n4,")
        color = call_module("r.colors.out", map=self.cell_ab_result).strip()
        table = (
            "0 0:0:0\n1 1:1:1\n2 2:2:2\n3 3:3:3\n4 4:4:4\n"
            "nv 255:255:255\ndefault 255:255:255"
        )
        self.assertMultiLineEqual(color, table)
        color = call_module("r.colors.out", map=self.cell_ab_result_threaded).strip()
        self.assertMultiLineEqual(color, table)
        color = call_module("r.colors.out", map=self.cell_ab_s_result).strip()
        self.assertNotEqual(color, table)
        color = call_module("r.colors.out", map=self.cell_ab_s_result_threaded).strip()
        self.assertNotEqual(color, table)


if __name__ == "__main__":
    test()
