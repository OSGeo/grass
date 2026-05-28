"""
Name:       r.basins.fill
Purpose:    Tests r.basins.fill and its flags/options.

Author:     Nishant Bansal
Copyright:  (C) 2025 by Nishant Bansal and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
import grass.script as gs


def get_categories(map_name):
    """
    Retrieve unique integer category values for a given raster map using r.stats.
    """
    stats = gs.read_command("r.stats", flags="nc", input=map_name, quiet=True).strip()
    return {int(line.split()[0]) for line in stats.splitlines() if line}


class TestRasterbasin(TestCase):
    """Test case for r.basins.fill module"""

    # Setup variables to be used for outputs
    streams = "test_streams"
    geomorphons = "test_geomorphons"
    ridges = "test_ridges"
    subbasins = "test_subbasins"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # Set the computational region based on the "elevation" map.
        cls.runModule("g.region", n=220000, s=218000, e=640000, w=632000, res=10)
        # Run r.watershed to generate the coded stream network.
        cls.runModule(
            "r.watershed",
            elevation="elevation",
            threshold=1000,
            stream=cls.streams,
        )
        # Ensure null values are replaced with 0 in streams
        cls.runModule("r.null", map=cls.streams, null=0)

        # Run r.geomorphon to generate geomorphometric forms.
        cls.runModule(
            "r.geomorphon",
            elevation="elevation",
            forms=cls.geomorphons,
            search=36,
            skip=6,
            quiet=True,
        )
        # Extract ridges from the geomorphon output.
        cls.runModule(
            "r.mapcalc", expression=f"{cls.ridges} = if({cls.geomorphons}==3,1,0)"
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary maps and region created during the test."""
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name=[cls.streams, cls.ridges, cls.subbasins, cls.geomorphons],
        )
        cls.del_temp_region()

    def test_r_basins_fill_method(self):
        """Test r.basins.fill to verify that it generates the correct watershed subbasins raster map."""

        self.assertModule(
            "r.basins.fill",
            cnetwork=self.streams,
            tnetwork=self.ridges,
            output=self.subbasins,
            number=2,
        )

        self.assertRasterExists(self.subbasins)
        self.assertRasterMinMax(
            map=self.subbasins,
            refmin=0,
            refmax=2482,
            msg="subbasins in degrees must be between 0 and 2482",
        )

        # Retrieve unique category values from the streams and subbasins maps.
        streams_cats = get_categories(self.streams)
        self.assertTrue(streams_cats, "No categories found in the 'streams' map.")

        subbasins_cats = get_categories(self.subbasins)
        self.assertTrue(subbasins_cats, "No categories found in the 'subbasins' map.")

        # Check that every nonzero channel code in subbasins is present in the streams map.
        self.assertTrue(
            subbasins_cats.issubset(streams_cats),
            "Missing basins for some stream codes",
        )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
