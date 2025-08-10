#!/usr/bin/env python3

"""
MODULE:    Test of r.surf.gauss

AUTHOR(S): Corey White <ctwhite48 gmail com>

PURPOSE: Tests random gauss surface generation

COPYRIGHT: (C) 2023 - 2024 by Corey White and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class MeanSigmaTestCase(TestCase):
    """Test r.surf.gauss module"""

    # Raster map name be used as output
    output = "random_result"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region"""
        os.environ["GRASS_RANDOM_SEED"] = "42"
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

    def test_defaut_settings(self):
        """Check to see if univariate statistics match for default"""
        self.assertModule("r.surf.gauss", output=self.output)
        self.assertRasterFitsUnivar(
            self.output,
            reference={"mean": -0.044860, "stddev": 1.019485},
            precision=1e-6,
        )

    def test_mean_sigma_params(self):
        """Check if mean and sigma params are accepted"""
        mean_value = 3.0
        sigma_value = 5.8
        self.assertModule(
            "r.surf.gauss",
            mean=mean_value,
            sigma=sigma_value,
            output=self.output,
        )
        self.assertRasterExists(self.output, msg="Output was not created")
        self.assertRasterFitsUnivar(
            self.output,
            reference={"mean": 2.739812, "stddev": 5.913014},
            precision=1e-6,
        )

    def test_random_seed_option(self):
        """Checks if random seed option sets random number"""
        mean_value = 3.0
        sigma_value = 5.8
        self.assertModule(
            "r.surf.gauss",
            mean=mean_value,
            sigma=sigma_value,
            output=self.output,
            seed=22,
        )
        self.assertRasterExists(self.output, msg="Output was not created")
        self.assertRasterFitsUnivar(
            self.output,
            reference={"mean": 3.183532, "stddev": 6.050756},
            precision=1e-6,
        )


if __name__ == "__main__":
    test()
