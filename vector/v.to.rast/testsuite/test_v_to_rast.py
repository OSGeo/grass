"""
Name:       v.to.rast test

Purpose:    Test v.to.rast

Author:     Vaclav Petras

SPDX-FileCopyrightText: 2023 Vaclav Petras
SPDX-FileCopyrightText: GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestParameters(TestCase):
    """Test v.to.rast"""

    output = "roads"

    @classmethod
    def setUpClass(cls):
        """Specify region for raster creation for this class"""
        cls.use_temp_region()
        cls.runModule("g.region", raster="roadsmajor", res=10, flags="a")

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove maps after each test method"""
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[self.output],
        )

    def test_legacy_use_interface(self):
        """Check that a legacy value for use parameter works"""
        self.assertModule(
            "v.to.rast", input="roadsmajor", output=self.output, use="val", value=1
        )

    def test_use_interface(self):
        """Check that use=value value=1 works"""
        self.assertModule(
            "v.to.rast", input="roadsmajor", output=self.output, use="value", value=1
        )
        self.assertRasterFitsInfo(raster=self.output, reference={"min": 1, "max": 1})


class TestDenseLineThroughCellCorner(TestCase):
    """A densified line is 4-connected when a vertex lies on a cell corner"""

    # The middle vertex of each line is exactly on a cell corner.
    lines = {
        # the segment after it is steeper than 45 degrees
        "dense_steep": "L 3 1\n 19.125 22.5\n 10 11\n 8.6074 9.2344\n 1 1\n",
        # the same line mirrored over the diagonal, so flatter than 45 degrees
        "dense_flat": "L 3 1\n 22.5 19.125\n 11 10\n 9.2344 8.6074\n 1 1\n",
    }
    output = "dense"
    clumps = "dense_clumps"

    @classmethod
    def setUpClass(cls):
        """Create the lines in a region with 1 x 1 cells"""
        cls.use_temp_region()
        cls.runModule("g.region", n=20, s=0, w=0, e=20, res=1)
        for name, text in cls.lines.items():
            cls.runModule(
                "v.in.ascii",
                flags="n",
                input="-",
                stdin_=text,
                output=name,
                format="standard",
            )

    @classmethod
    def tearDownClass(cls):
        """Remove the lines and the temporary region"""
        cls.runModule("g.remove", flags="f", type="vector", name=list(cls.lines))
        cls.del_temp_region()

    def tearDown(self):
        """Remove the rasters after each test method"""
        self.runModule(
            "g.remove", flags="f", type="raster", name=[self.output, self.clumps]
        )

    def test_dense_line_is_one_piece(self):
        """Each line is one clump without diagonal neighbours, in 20 cells

        The two lines are mirror images, so they take as many cells. Before the
        fix, the steep one had a gap and the flat one ran along a single row.
        """
        for name in self.lines:
            with self.subTest(line=name):
                self.assertModule(
                    "v.to.rast",
                    flags="d",
                    input=name,
                    output=self.output,
                    use="val",
                    overwrite=True,
                )
                self.assertRasterFitsUnivar(raster=self.output, reference={"n": 20})
                self.assertModule(
                    "r.clump", input=self.output, output=self.clumps, overwrite=True
                )
                self.assertRasterFitsInfo(
                    raster=self.clumps, reference={"min": 1, "max": 1}
                )


if __name__ == "__main__":
    test()
