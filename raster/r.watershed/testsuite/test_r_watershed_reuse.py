"""
Name:      r_watershed_reuse_test
Purpose:   Unit tests for r.watershed's ability to reuse existing accumulation
           and drainage maps instead of recalculating them.

Author:    Sumit Chintanwar
Copyright: (C) 2026 by Sumit Chintanwar and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestWatershedReuse(TestCase):
    """Test r.watershed input map reuse functionality"""

    accumulation = "reuse_accum"
    drainage = "reuse_drain"
    elevation = "reuse_elevation"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()

        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.elevation} = row() + col()",
        )

        cls.runModule("g.region", raster=cls.elevation, res=10)

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", flags="f", type="raster", name=cls.elevation)
        cls.del_temp_region()

    def tearDown(self):
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            pattern="reuse_*",
            exclude=self.elevation,
        )

    def test_reuse_workflow(self):
        """Test complete reuse workflow: generate, reuse for accumulation and RUSLE"""
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            accumulation=self.accumulation,
            drainage=self.drainage,
            threshold=1000,
        )

        self.assertRasterExists(self.accumulation)
        self.assertRasterExists(self.drainage)

        self.assertRasterMinMax(
            self.drainage,
            refmin=-8,
            refmax=8,
            msg="Drainage direction must be between -8 and 8",
        )

        accum_out = "reuse_accum_out"
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            accumulation_input=self.accumulation,
            drainage_input=self.drainage,
            accumulation=accum_out,
        )

        self.assertRastersNoDifference(
            actual=accum_out,
            reference=self.accumulation,
            precision=0.01,
            msg="Reused accumulation should match original",
        )

        ls_factor = "reuse_ls"
        self.assertModule(
            "r.watershed",
            elevation=self.elevation,
            accumulation_input=self.accumulation,
            drainage_input=self.drainage,
            threshold=1000,
            length_slope=ls_factor,
            max_slope_length=100,
        )

        self.assertRasterExists(ls_factor)


if __name__ == "__main__":
    test()
