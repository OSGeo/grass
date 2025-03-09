import numpy as np
import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestRRandomSurface(TestCase):
    output_raster = "random_surface_test"
    mask_raster = "random_surface_mask"

    @classmethod
    def setUpClass(cls):
        """Setup input rasters and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=20, s=0, e=20, w=0, rows=20, cols=20)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.mask_raster} = if(row() < 10, 1, null())",
            quiet=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove generated rasters and reset test environment."""
        cls.runModule(
            "g.remove",
            type="raster",
            name=[cls.mask_raster],
            flags="f",
            quiet=True,
        )
        cls.del_temp_region()

    def tearDown(self):
        """Remove the created rasters after each test."""
        self.runModule(
            "g.remove",
            type="raster",
            name=self.output_raster,
            flags="f",
            quiet=True,
        )

    def test_seed_reproducibility(self):
        """Check reproducibility using the same seed."""
        self.runModule(
            "r.random.surface", output=self.output_raster, seed=42, overwrite=True
        )
        result1 = gs.read_command(
            "r.stats", input=self.output_raster, flags="n", quiet=True
        )

        self.runModule(
            "r.random.surface", output=self.output_raster, seed=42, overwrite=True
        )
        result2 = gs.read_command(
            "r.stats", input=self.output_raster, flags="n", quiet=True
        )

        self.assertEqual(result1, result2, "Outputs with the same seed differ")

    def test_with_mask(self):
        """Test with a raster mask applied and validate stats."""
        self.runModule("r.mask", raster=self.mask_raster)
        self.assertModule(
            "r.random.surface", output=self.output_raster, seed=42, overwrite=True
        )
        self.runModule("r.mask", flags="r")

        reference_stats = {
            "mean": 135.695,
            "min": 1,
            "max": 255,
            "cells": 400,
        }

        self.assertRasterFitsUnivar(
            raster=self.output_raster, reference=reference_stats, precision=1e-6
        )

    def test_different_seeds(self):
        """Test that different seeds produce different outputs."""
        self.runModule(
            "r.random.surface", output=self.output_raster, seed=42, overwrite=True
        )
        result1 = gs.read_command(
            "r.stats", input=self.output_raster, flags="n", quiet=True
        )

        self.runModule(
            "r.random.surface", output=self.output_raster, seed=43, overwrite=True
        )
        result2 = gs.read_command(
            "r.stats", input=self.output_raster, flags="n", quiet=True
        )

        self.assertNotEqual(
            result1, result2, "Different seeds produced identical outputs"
        )

    def test_distance_parameters(self):
        """Test with different distance parameters and compare to reference statistics."""

        self.runModule(
            "r.random.surface",
            output=self.output_raster,
            distance=5.0,
            exponent=3.0,
            seed=43,
            overwrite=True,
        )

        reference_stats = {
            "mean": 129.45,
            "min": 1,
            "max": 255,
            "cells": 400,
            "stddev": 53.109015,
        }

        self.assertRasterFitsUnivar(
            raster=self.output_raster, reference=reference_stats, precision=1e-6
        )

    def test_distribution_properties(self):
        """Test statistical properties of the generated random surface."""
        self.runModule(
            "r.random.surface", output=self.output_raster, seed=42, overwrite=True
        )
        stats = gs.parse_command("r.univar", map=self.output_raster, format="json")

        n = stats[0]["n"]
        mean = stats[0]["mean"]
        stddev = stats[0]["stddev"]

        self.assertGreater(n, 0, "No valid cells in output")
        self.assertTrue(0 <= mean <= 255, "Mean outside expected range")
        self.assertGreater(stddev, 0, "No variation in output values")

    def test_custom_high(self):
        """Test with custom high value."""
        high_val = 100
        self.runModule(
            "r.random.surface", output=self.output_raster, high=high_val, overwrite=True
        )

        stats = gs.parse_command("r.univar", map=self.output_raster, format="json")

        actual_min = stats[0]["min"]
        actual_max = stats[0]["max"]

        self.assertGreaterEqual(
            actual_min, 0, f"Minimum value {actual_min} is less than 0"
        )
        self.assertLessEqual(
            actual_max,
            high_val,
            f"Maximum value {actual_max} is greater than specified high value {high_val}",
        )

    def test_regional_correlation(self):
        """Test spatial correlation properties using r.stats"""
        self.runModule(
            "r.random.surface",
            output=self.output_raster,
            distance=5.0,
            exponent=3.0,
            seed=42,
            overwrite=True,
        )

        output_stats = f"{self.output_raster}_stats"
        self.runModule(
            "r.mapcalc",
            expression=f"{output_stats} = {self.output_raster}[-1,-1]",
            quiet=True,
        )

        stats_output = gs.read_command(
            "r.stats",
            input=f"{self.output_raster},{output_stats}",
            flags="c",
            quiet=True,
        ).splitlines()

        self.assertGreater(
            len(stats_output), 0, "r.stats -c output is empty or malformed"
        )

        frequencies = [line.split() for line in stats_output if len(line.split()) == 3]

        self.assertGreater(
            len(frequencies), 0, "No valid frequency data extracted from r.stats -c"
        )

        values = np.array([float(f[0]) for f in frequencies])
        counts = np.array([int(f[2]) for f in frequencies])

        correlation = np.corrcoef(values, counts)[0, 1]
        self.assertGreater(
            correlation,
            0,
            "No positive spatial correlation detected with distance parameter",
        )

        self.runModule(
            "g.remove", type="raster", name=output_stats, flags="f", quiet=True
        )

    def test_uniform_distribution(self):
        """Test that -u flag produces uniformly distributed values."""
        self.runModule(
            "r.random.surface",
            output=self.output_raster,
            seed=42,
            flags="u",
            overwrite=True,
        )

        stats = gs.parse_command("r.univar", map=self.output_raster, format="json")

        n = stats[0]["n"]
        min_val = stats[0]["min"]
        max_val = stats[0]["max"]
        mean = stats[0]["mean"]

        expected_mean = (min_val + max_val) / 2
        mean_tolerance = 25  # Kept high to account for randomness

        self.assertGreater(n, 0, "No valid cells in output")
        self.assertGreaterEqual(min_val, 0, "Minimum value below expected range")
        self.assertLessEqual(max_val, 255, "Maximum value above expected range")
        self.assertAlmostEqual(
            mean,
            expected_mean,
            delta=mean_tolerance,
            msg="Mean deviates significantly from expected uniform distribution mean",
        )

        range_expr = [
            f"if({self.output_raster} <= 63.75, 1, 0)",
            f"if({self.output_raster} > 63.75 && {self.output_raster} <= 127.5, 1, 0)",
            f"if({self.output_raster} > 127.5 && {self.output_raster} <= 191.25, 1, 0)",
            f"if({self.output_raster} > 191.25, 1, 0)",
        ]

        counts = []
        for i, expr in enumerate(range_expr):
            tmp_map = f"tmp_range_{i}"
            self.runModule("r.mapcalc", expression=f"{tmp_map} = {expr}", quiet=True)
            stats = gs.parse_command("r.univar", map=tmp_map, format="json")
            counts.append(stats[0]["sum"])

            self.runModule(
                "g.remove", type="raster", name=tmp_map, flags="f", quiet=True
            )

        expected_count = n / 4
        tolerance = expected_count * 0.25

        for i, count in enumerate(counts):
            self.assertAlmostEqual(
                count,
                expected_count,
                delta=tolerance,
                msg=f"Count in range {i + 1} deviates significantly from uniform distribution",
            )


if __name__ == "__main__":
    test()
