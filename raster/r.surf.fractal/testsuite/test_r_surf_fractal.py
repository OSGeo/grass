#!/usr/bin/env python3

"""
MODULE:    Test of r.surf.fractal

AUTHOR(S): Nishant Bansal <nishant.bansal.282003@gmail.com>

PURPOSE: Test fractal surface generation of a given fractal dimension.

COPYRIGHT: (C) 2025 by Nishant Bansal and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRSurfFractal(TestCase):
    """Test case for r.surf.fractal"""

    # Raster map name to be used as output
    output = "test_fractal"

    @classmethod
    def setUpClass(cls):
        """Set up necessary environment"""
        # Set up temporary computational region
        cls.use_temp_region()
        cls.runModule("g.region", rows=10, cols=10)

    @classmethod
    def tearDownClass(cls):
        """Clean up temporary environment"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the output created from the module after each test"""
        self.runModule("g.remove", flags="f", type="raster", name=[self.output])

    def test_default_settings(self):
        """Test r.surf.fractal with default settings."""
        self.assertModule("r.surf.fractal", output=self.output)
        self.assertRasterExists(self.output, msg="Output Raster not created")

    def test_dimension_number_params(self):
        """Test r.surf.fractal with the specified dimension and number parameters."""
        os.environ["GRASS_RANDOM_SEED"] = "42"
        fractal_dim = 2.0005
        num_images = 2

        self.assertModule(
            "r.surf.fractal",
            dimension=fractal_dim,
            number=num_images,
            output=self.output,
        )

        self.assertRasterExists(self.output, msg="Output Raster not created")
        self.assertRasterFitsUnivar(
            self.output,
            reference={"mean": -3693.824428, "stddev": 12477.543361},
            precision=1e-6,
        )

    def test_random_seed_option(self):
        """Test r.surf.fractal checks whether the random seed option sets a random number."""
        fractal_dim = 2.0005
        num_images = 2
        seed_value = 22

        self.assertModule(
            "r.surf.fractal",
            dimension=fractal_dim,
            number=num_images,
            seed=seed_value,
            output=self.output,
        )

        self.assertRasterExists(self.output, msg="Output was not created")
        self.assertRasterFitsUnivar(
            self.output,
            reference={"mean": -925.148233, "stddev": 19416.071318},
            precision=1e-6,
        )


if __name__ == "__main__":
    test()
