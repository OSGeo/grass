#!/usr/bin/env python3

"""
MODULE:    Test of r.surf.random

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test of min and max parameters

COPYRIGHT: (C) 2020 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import grass.script as gs

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


def get_raster_min_max(raster_map):
    """Get minimum and maximum value from raster metadata as a tuple"""
    info = gs.raster_info(raster_map)
    return info["min"], info["max"]


class MinMaxTestCase(TestCase):
    """Test min and max of r.surf.random module"""

    # Raster map name be used as output
    output = "random_result"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region"""
        # modifying region just for this script
        cls.use_temp_region()
        cls.runModule("g.region", rows=10, cols=10)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the output created from the module"""
        self.runModule("g.remove", flags="f", type="raster", name=[self.output])

    def test_min_max_double(self):
        """Check to see if double output has the expected range"""
        min_value = -3.3
        max_value = 5.8
        precision = 0.00001
        self.assertModule(
            "r.surf.random", min=min_value, max=max_value, output=self.output, seed=42
        )
        self.assertRasterExists(self.output, msg="Output was not created")
        self.assertRasterMinMax(
            map=self.output,
            refmin=min_value,
            refmax=max_value,
            msg="Output exceeds the min and max values from parameters",
        )
        self.assertRasterFitsInfo(
            raster=self.output,
            reference={"min": -3.20423, "max": 5.68621},
            precision=precision,
            msg="Output min and max too far from parameters",
        )

    def test_min_max_int(self):
        """Check to see if integer output has the expected range"""
        # GRASS_RANDOM_SEED=42 r.surf.random output=test min=-2 max=13 -i
        # in 7.6.2 causes all no 2 bin and extra 14 bin (also doubles 0).
        min_value = -2
        max_value = 13
        precision = 0
        self.assertModule(
            "r.surf.random",
            min=min_value,
            max=max_value,
            output=self.output,
            seed=42,
            flags="i",
        )
        self.assertRasterExists(self.output, msg="Output was not created")
        self.assertRasterMinMax(
            map=self.output,
            refmin=min_value,
            refmax=max_value,
            msg="Output exceeds the min and max values from parameters",
        )
        self.assertRasterFitsInfo(
            raster=self.output,
            reference={"min": min_value, "max": max_value},
            precision=precision,
            msg="Output min and max too far from parameters",
        )

    def test_double_params_with_int(self):
        """Check if doubles instead of ints are refused"""
        min_value = -3.3
        max_value = 5.8
        self.assertModuleFail(
            "r.surf.random",
            min=min_value,
            max=max_value,
            output=self.output,
            seed=42,
            flags="i",
        )

    def test_min_greater_than_max(self):
        """Check if minimum greater than maximum is refused"""
        min_value = 10
        max_value = 5.8
        self.assertModuleFail(
            "r.surf.random", min=min_value, max=max_value, output=self.output, seed=42
        )

    def test_auto_seed(self):
        """Check if random seed is generated without seed"""
        min_value = -3.3
        max_value = 5.8
        self.assertModule(
            "r.surf.random", min=min_value, max=max_value, output=self.output
        )


if __name__ == "__main__":
    test()
