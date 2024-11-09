#!/usr/bin/env python3

"""
MODULE:    Test of r.surf.fractal

AUTHOR(S): Shashank Shekhar Singh <shashankshekharsingh1205 gmail com>

PURPOSE:   Test fractal surface generation with r.surf.fractal module

COPYRIGHT: (C) 2024 by Shashank Shekhar Singh and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""


from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class FractalTestCase(TestCase):
    """Test case for r.surf.fractal"""

    # Raster map name to be used as output
    output = "fractal_result"

    @classmethod
    def setUpClass(cls):
        """Set up necessary environment"""
        # Set up temporary computational region
        cls.use_temp_region()
        # Only 100,000,000 seem to reasonably (not 100%) ensure that all values
        # are generated, so exceeding of ranges actually shows up.
        cls.runModule("g.region", rows=10000, cols=10000)

    @classmethod
    def tearDownClass(cls):
        """Clean up temporary environment"""
        cls.del_temp_region()

    def tearDown(self):
        """Remove the output created from the module after each test"""
        self.runModule("g.remove", flags="f", type="raster", name=[self.output])

    def test_default_settings(self):
        """Test with default settings"""
        self.assertModule("r.surf.fractal", output=self.output)

    def test_fractal_dimension(self):
        """Test with specified fractal dimension"""
        fractal_dim = 2.05  # Example fractal dimension value
        self.assertModule(
            "r.surf.fractal",
            dimension=fractal_dim,
            output=self.output,
        )

    def test_num_images(self):
        """Test with specified number of intermediate images"""
        num_images = 5  # Example number of intermediate images
        self.assertModule(
            "r.surf.fractal",
            number=num_images,
            output=self.output,
        )

    def test_random_seed(self):
        """Test with specified random seed"""
        seed_value = 12345  # Example random seed value
        self.assertModule(
            "r.surf.fractal",
            seed=seed_value,
            output=self.output,
        )

    def test_generate_random_seed(self):
        """Test with flag to generate random seed"""
        self.assertModule(
            "r.surf.fractal",
            flags="s",
            output=self.output,
        )

    def test_invalid_fractal_dimension(self):
        """Test with invalid fractal dimension"""
        invalid_dim = 1.5  # Example invalid fractal dimension value
        # Ensure that the module fails with an error message
        self.assertModuleFail(
            "r.surf.fractal",
            dimension=invalid_dim,
            output=self.output,
        )

    def test_invalid_seed_value(self):
        """Test with invalid random seed value"""
        invalid_seed = "abc"  # Example invalid random seed value
        # Ensure that the module fails with an error message
        self.assertModuleFail(
            "r.surf.fractal",
            seed=invalid_seed,
            output=self.output,
        )


if __name__ == "__main__":
    test()
