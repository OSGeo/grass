#!/usr/bin/env python3

"""
MODULE:    Test of r.surf.gauss

AUTHOR(S): Corey White <ctwhite48 gmail com>

PURPOSE: Tests random gauss surface generation

COPYRIGHT: (C) 2023 by Corey White and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class MeanSigmaTestCase(TestCase):
    """Test min and max of r.surf.random module"""

    # Raster map name be used as output
    output = "random_result"

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region"""
        os.environ["GRASS_RANDOM_SEED"] = "42"
        # modifying region just for this script
        cls.use_temp_region()
        # Only 100,000,000 seem to resonably (not 100%) ensure that all values
        # are generated, so exceeding of ranges actually shows up.
        cls.runModule("g.region", rows=10000, cols=10000)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the output created from the module"""
        self.runModule("g.remove", flags="f", type="raster", name=[self.output])

    def test_defaut_settings(self):
        """Check to see if double output has the expected range"""
        default_mean = 0.0
        default_sigma = 1.0
        self.assertModule("r.surf.gauss", output=self.output)
        self.assertRasterFitsUnivar(
            self.output,
            reference=dict(mean=default_mean, stddev=default_sigma),
            msg="The mean and sigma values are not within the expected range.",
            precision=0.01,
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

    def test_random_flag(self):
        """Checks if random flag sets random number"""
        mean_value = 3.0
        sigma_value = 5.8
        self.assertModule(
            "r.surf.gauss",
            mean=mean_value,
            sigma=sigma_value,
            output=self.output,
            flags="s",
        )

        self.assertRasterExists(self.output, msg="Output was not created")
        self.assertRasterFitsUnivar(
            self.output,
            reference=dict(mean=mean_value, stddev=sigma_value),
            msg="The mean and sigma values are not within the expected range.",
            precision=0.01,
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
            reference=dict(mean=mean_value, stddev=sigma_value),
            msg="The mean and sigma values are not within the expected range.",
            precision=0.01,
        )


if __name__ == "__main__":
    test()
