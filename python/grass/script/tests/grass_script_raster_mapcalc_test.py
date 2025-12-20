"""Tests for grass.script.raster mapcalc functions"""

import pytest
import numpy as np
import re

import grass.script as gs
from grass.pygrass.raster import raster2numpy


def extract_seed_from_comments(comments_str):
    """Extract explicit numeric seed from raster comments/metadata.

    Parses the seed value from r.mapcalc metadata comments string.
    Searches for patterns like 'seed=12345' or 'seed: 12345' (handles optional spacing).

    Args:
        comments_str: The comments/metadata string from raster_info()

    Returns:
        int: The seed value if found, None otherwise
    """
    if not comments_str:
        return None
    match = re.search(r"\bseed\s*[=:]\s*(-?\d+)", comments_str)
    return int(match.group(1)) if match else None


class TestMapcalcRandFunction:
    """Tests for rand() function in mapcalc

    r.mapcalc auto-seeds when no seed is provided. Explicit seed can be used for reproducibility.
    """

    @pytest.fixture(autouse=True)
    def setup_region(self, session_2x2):
        """Setup region for tests"""
        self.session = session_2x2
        gs.run_command("g.region", rows=10, cols=10, env=self.session.env)

    def teardown_method(self):
        """Clean up maps after each test using pattern matching"""
        try:
            maps = gs.list_strings(
                type="raster", pattern="rand_map*", env=self.session.env
            )
            if maps:
                gs.run_command(
                    "g.remove",
                    type="raster",
                    flags="f",
                    name=",".join(maps),
                    env=self.session.env,
                    quiet=True,
                )
        except Exception:
            # Silently ignore cleanup errors to avoid masking test failures
            pass

    def test_mapcalc_rand_autoseeded(self):
        """Test that rand() auto-seeds when no explicit seed is provided.

        Verifies:
        1. Two consecutive runs without explicit seed produce different results (different auto-seeds)
        2. Metadata contains different auto-generated seed values
        """
        # r.mapcalc auto-seeds when no seed is given — two runs should differ
        gs.mapcalc(
            "rand_map_auto1 = rand(0.0, 1.0)",
            env=self.session.env,
        )
        gs.mapcalc(
            "rand_map_auto2 = rand(0.0, 1.0)",
            env=self.session.env,
        )

        # Verify arrays differ (outcome of different auto-seeds)
        array1 = raster2numpy("rand_map_auto1")
        array2 = raster2numpy("rand_map_auto2")
        assert array1 is not None
        assert array2 is not None
        assert array1.size > 0
        assert array2.size > 0
        assert not np.array_equal(array1, array2), (
            "Auto-seeded runs should produce different maps"
        )

        # Verify mechanism: auto-generated seeds are recorded in metadata and differ
        info1 = gs.raster_info("rand_map_auto1", env=self.session.env)
        info2 = gs.raster_info("rand_map_auto2", env=self.session.env)
        comments1 = info1.get("comments", "")
        comments2 = info2.get("comments", "")

        seed1 = extract_seed_from_comments(comments1)
        seed2 = extract_seed_from_comments(comments2)

        assert seed1 is not None, (
            "Auto-seed should be recorded in metadata for first run"
        )
        assert seed2 is not None, (
            "Auto-seed should be recorded in metadata for second run"
        )
        assert seed1 != seed2, "Auto-seeds should differ between runs"

    def test_mapcalc_rand_with_explicit_seed(self):
        """Test that rand() works with explicit seed value.

        Verifies:
        1. Seed parameter is passed to r.mapcalc
        2. Metadata contains the explicit seed value
        """
        gs.mapcalc(
            "rand_map_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        # Check that the map was created and seed was written to metadata
        raster_info = gs.raster_info("rand_map_seed", env=self.session.env)
        assert raster_info is not None

        # Verify seed parameter was passed to r.mapcalc (check metadata)
        comments = raster_info.get("comments", "")
        seed_found = extract_seed_from_comments(comments)
        assert seed_found == 12345, (
            f"Expected seed=12345 in metadata, got seed={seed_found}"
        )

    def test_mapcalc_rand_with_seed_auto_backwards_compat(self):
        """Test that seed='auto' is handled for backwards compatibility.

        seed='auto' should be converted to None internally, enabling auto-seeding behavior.
        This means it should behave identically to not providing a seed parameter.

        Verifies:
        1. seed='auto' does not pass literal 'auto' to r.mapcalc (it's converted before calling)
        2. Two runs with seed='auto' produce different results (auto-seeding enabled)
        3. Auto-generated seeds are recorded in metadata (consistent with no-seed behavior)
        4. Auto-generated seeds differ between runs
        """
        # Create one map with seed='auto'
        gs.mapcalc(
            "rand_map_auto_seed = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        raster_info_1 = gs.raster_info("rand_map_auto_seed", env=self.session.env)
        assert raster_info_1 is not None
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)

        # Since seed='auto' converts to None, it should enable auto-seeding
        # Auto-seeding records generated seed in metadata (same as no-seed case)
        assert seed_1 is not None, (
            "seed='auto' should convert to None and enable auto-seeding, recording an auto-generated seed in metadata"
        )

        # Create second map with seed='auto' — should differ from first (different auto-seeds)
        gs.mapcalc(
            "rand_map_auto_seed_2 = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        raster_info_2 = gs.raster_info("rand_map_auto_seed_2", env=self.session.env)
        assert raster_info_2 is not None
        comments_2 = raster_info_2.get("comments", "")
        seed_2 = extract_seed_from_comments(comments_2)

        # Verify both runs have auto-generated seeds and they differ
        assert seed_2 is not None, (
            "seed='auto' should enable auto-seeding, recording an auto-generated seed"
        )
        assert seed_1 != seed_2, (
            "seed='auto' should produce different auto-generated seeds on consecutive runs"
        )

        # Verify actual raster data differs
        array_1 = raster2numpy("rand_map_auto_seed")
        array_2 = raster2numpy("rand_map_auto_seed_2")
        assert not np.array_equal(array_1, array_2, equal_nan=True), (
            "seed='auto' should enable auto-seeding, producing different results each time"
        )

    def test_mapcalc_rand_with_explicit_seed_reproducible(self):
        """Test that explicit seed produces reproducible results.

        Verifies:
        1. Same seed produces identical raster arrays (byte-for-byte reproducibility)
        2. Different seeds produce different results (seed actually has effect)
        3. Seed metadata is correctly recorded for all variations
        """
        # Create two maps with same seed=12345
        gs.mapcalc(
            "rand_map_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        raster_info_1 = gs.raster_info("rand_map_seed", env=self.session.env)
        assert raster_info_1 is not None
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)
        assert seed_1 == 12345, f"Expected seed=12345 in metadata, got seed={seed_1}"

        # Read actual raster data
        array_1 = raster2numpy("rand_map_seed")
        assert array_1 is not None
        assert array_1.size > 0

        # Create another map with the same seed
        gs.mapcalc(
            "rand_map_seed_2 = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        raster_info_2 = gs.raster_info("rand_map_seed_2", env=self.session.env)
        assert raster_info_2 is not None
        comments_2 = raster_info_2.get("comments", "")
        seed_2 = extract_seed_from_comments(comments_2)
        assert seed_2 == 12345, f"Expected seed=12345 in metadata, got seed={seed_2}"

        # Read actual raster data
        array_2 = raster2numpy("rand_map_seed_2")
        assert array_2 is not None
        assert array_2.size > 0

        # Verify reproducibility: arrays must be identical (byte-for-byte)
        assert np.array_equal(array_1, array_2, equal_nan=True), (
            "Maps with same seed should have identical cell values"
        )

        # Verify seed actually has an effect by comparing with different seed
        gs.mapcalc(
            "rand_map_seed_3 = rand(0.0, 1.0)",
            seed=54321,
            env=self.session.env,
        )
        raster_info_3 = gs.raster_info("rand_map_seed_3", env=self.session.env)
        comments_3 = raster_info_3.get("comments", "")
        seed_3 = extract_seed_from_comments(comments_3)
        assert seed_3 == 54321, f"Expected seed=54321 in metadata, got seed={seed_3}"

        array_3 = raster2numpy("rand_map_seed_3")
        assert array_3 is not None

        # Different seed should produce different results
        assert not np.array_equal(array_1, array_3, equal_nan=True), (
            "Maps with different seeds should have different cell values"
        )

    def test_mapcalc_rand_with_seed_zero(self):
        """Test that seed=0 is handled correctly.

        Verifies:
        1. seed=0 is valid and produces reproducible results
        2. seed=0 differs from seed='auto' (explicit seed produces different results than auto-seeding)
        """
        # Create map with seed=0
        gs.mapcalc(
            "rand_map_seed_0 = rand(0.0, 1.0)",
            seed=0,
            env=self.session.env,
        )
        raster_info_1 = gs.raster_info("rand_map_seed_0", env=self.session.env)
        assert raster_info_1 is not None
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)
        assert seed_1 == 0, f"Expected seed=0 in metadata, got seed={seed_1}"

        array_1 = raster2numpy("rand_map_seed_0")

        # Create another map with seed=0 — should be reproducible
        gs.mapcalc(
            "rand_map_seed_0_2 = rand(0.0, 1.0)",
            seed=0,
            env=self.session.env,
        )
        array_2 = raster2numpy("rand_map_seed_0_2")
        assert np.array_equal(array_1, array_2, equal_nan=True), (
            "seed=0 should produce reproducible results"
        )

        # Verify seed=0 differs from auto-seeding
        gs.mapcalc(
            "rand_map_seed_0_auto = rand(0.0, 1.0)",
            env=self.session.env,
        )
        array_auto = raster2numpy("rand_map_seed_0_auto")
        assert not np.array_equal(array_1, array_auto, equal_nan=True), (
            "seed=0 should produce different results than auto-seeding"
        )

    def test_mapcalc_rand_with_negative_seed(self):
        """Test that negative seeds are handled correctly.

        Verifies:
        1. Negative seed values are valid and produce reproducible results
        2. Different negative seeds produce different results
        """
        # Create map with negative seed
        gs.mapcalc(
            "rand_map_neg_seed = rand(0.0, 1.0)",
            seed=-42,
            env=self.session.env,
        )
        raster_info_1 = gs.raster_info("rand_map_neg_seed", env=self.session.env)
        assert raster_info_1 is not None
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)
        assert seed_1 == -42, f"Expected seed=-42 in metadata, got seed={seed_1}"

        array_1 = raster2numpy("rand_map_neg_seed")

        # Create another map with same negative seed — should be reproducible
        gs.mapcalc(
            "rand_map_neg_seed_2 = rand(0.0, 1.0)",
            seed=-42,
            env=self.session.env,
        )
        array_2 = raster2numpy("rand_map_neg_seed_2")
        assert np.array_equal(array_1, array_2, equal_nan=True), (
            "Negative seed should produce reproducible results"
        )

        # Different negative seed should produce different results
        gs.mapcalc(
            "rand_map_neg_seed_3 = rand(0.0, 1.0)",
            seed=-99,
            env=self.session.env,
        )
        array_3 = raster2numpy("rand_map_neg_seed_3")
        assert not np.array_equal(array_1, array_3, equal_nan=True), (
            "Different negative seeds should produce different results"
        )

    def test_mapcalc_rand_with_large_seed(self):
        """Test that very large seed values are handled correctly.

        Verifies:
        1. Large seed values are valid and produce reproducible results
        2. Large seed differs from regular seeds (seed value actually has effect)
        """
        large_seed = 2147483647  # Max 32-bit signed integer

        # Create map with large seed
        gs.mapcalc(
            "rand_map_large_seed = rand(0.0, 1.0)",
            seed=large_seed,
            env=self.session.env,
        )
        raster_info_1 = gs.raster_info("rand_map_large_seed", env=self.session.env)
        assert raster_info_1 is not None
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)
        assert seed_1 == large_seed, (
            f"Expected seed={large_seed} in metadata, got seed={seed_1}"
        )

        array_1 = raster2numpy("rand_map_large_seed")

        # Create another map with same large seed — should be reproducible
        gs.mapcalc(
            "rand_map_large_seed_2 = rand(0.0, 1.0)",
            seed=large_seed,
            env=self.session.env,
        )
        array_2 = raster2numpy("rand_map_large_seed_2")
        assert np.array_equal(array_1, array_2, equal_nan=True), (
            "Large seed should produce reproducible results"
        )

        # Verify large seed differs from regular seed (seed value has effect)
        gs.mapcalc(
            "rand_map_regular_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        array_regular = raster2numpy("rand_map_regular_seed")
        assert not np.array_equal(array_1, array_regular, equal_nan=True), (
            "Large seed should produce different results than regular seed"
        )


class TestMapcalcStartRandFunction:
    """Tests for rand() function in mapcalc_start

    r.mapcalc auto-seeds when no seed is provided. Explicit seed can be used for reproducibility.

    Note: Edge case tests (seed=0, negative seeds, large seeds) are only in TestMapcalcRandFunction
    because mapcalc and mapcalc_start share the same underlying seed handling implementation.
    Testing the mechanism once is sufficient to ensure both code paths work correctly.
    """

    @pytest.fixture(autouse=True)
    def setup_region(self, session_2x2):
        """Setup region for tests"""
        self.session = session_2x2
        gs.run_command("g.region", rows=10, cols=10, env=self.session.env)

    def teardown_method(self):
        """Clean up maps after each test using pattern matching"""
        try:
            maps = gs.list_strings(
                type="raster", pattern="rand_map_start*", env=self.session.env
            )
            if maps:
                gs.run_command(
                    "g.remove",
                    type="raster",
                    flags="f",
                    name=",".join(maps),
                    env=self.session.env,
                    quiet=True,
                )
        except Exception:
            # Silently ignore cleanup errors to avoid masking test failures
            pass

    def test_mapcalc_start_rand_autoseeded(self):
        """Test that mapcalc_start with rand() auto-seeds when no explicit seed is provided.

        Verifies:
        1. Two consecutive runs without explicit seed produce different results (different auto-seeds)
        2. Metadata contains different auto-generated seed values
        """
        # r.mapcalc auto-seeds when no seed is given — two runs should differ
        p = gs.mapcalc_start(
            "rand_map_start_auto1 = rand(0.0, 1.0)",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        p = gs.mapcalc_start(
            "rand_map_start_auto2 = rand(0.0, 1.0)",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0

        # Verify outcome: arrays differ (different auto-seeds)
        array1 = raster2numpy("rand_map_start_auto1")
        array2 = raster2numpy("rand_map_start_auto2")
        assert array1 is not None
        assert array2 is not None
        assert array1.size > 0
        assert array2.size > 0
        assert not np.array_equal(array1, array2), (
            "Auto-seeded runs should produce different maps"
        )

        # Verify mechanism: auto-generated seeds are recorded in metadata and differ
        info1 = gs.raster_info("rand_map_start_auto1", env=self.session.env)
        info2 = gs.raster_info("rand_map_start_auto2", env=self.session.env)
        comments1 = info1.get("comments", "")
        comments2 = info2.get("comments", "")

        seed1 = extract_seed_from_comments(comments1)
        seed2 = extract_seed_from_comments(comments2)

        assert seed1 is not None, (
            "Auto-seed should be recorded in metadata for first run"
        )
        assert seed2 is not None, (
            "Auto-seed should be recorded in metadata for second run"
        )
        assert seed1 != seed2, "Auto-seeds should differ between runs"

    def test_mapcalc_start_rand_with_explicit_seed(self):
        """Test that mapcalc_start with rand() works with explicit seed value.

        Verifies:
        1. Seed parameter is passed to r.mapcalc
        2. Metadata contains the explicit seed value
        """
        p = gs.mapcalc_start(
            "rand_map_start_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        # Verify the map was created and seed was written to metadata
        raster_info = gs.raster_info("rand_map_start_seed", env=self.session.env)
        assert raster_info is not None

        # Verify seed parameter was passed to r.mapcalc (check metadata)
        comments = raster_info.get("comments", "")
        seed_found = extract_seed_from_comments(comments)
        assert seed_found == 12345, (
            f"Expected seed=12345 in metadata, got seed={seed_found}"
        )

    def test_mapcalc_start_rand_with_seed_auto_backwards_compat(self):
        """Test that seed='auto' is handled for backwards compatibility.

        seed='auto' should be converted to None internally, enabling auto-seeding behavior.

        Verifies:
        1. seed='auto' does not pass literal 'auto' to r.mapcalc (it's converted before calling)
        2. Two runs with seed='auto' produce different results (auto-seeding enabled)
        3. Auto-generated seeds are recorded in metadata (consistent with no-seed behavior)
        4. Auto-generated seeds differ between runs
        """
        # Create one map with seed='auto'
        p = gs.mapcalc_start(
            "rand_map_start_auto_seed = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        raster_info_1 = gs.raster_info("rand_map_start_auto_seed", env=self.session.env)
        assert raster_info_1 is not None
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)

        # Since seed='auto' converts to None, it should enable auto-seeding
        # Auto-seeding records generated seed in metadata (same as no-seed case)
        assert seed_1 is not None, (
            "seed='auto' should convert to None and enable auto-seeding, recording an auto-generated seed in metadata"
        )

        # Create second map with seed='auto' — should differ from first (different auto-seeds)
        p = gs.mapcalc_start(
            "rand_map_start_auto_seed_2 = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        raster_info_2 = gs.raster_info(
            "rand_map_start_auto_seed_2", env=self.session.env
        )
        assert raster_info_2 is not None
        comments_2 = raster_info_2.get("comments", "")
        seed_2 = extract_seed_from_comments(comments_2)

        # Verify both runs have auto-generated seeds and they differ
        assert seed_2 is not None, (
            "seed='auto' should enable auto-seeding, recording an auto-generated seed"
        )
        assert seed_1 != seed_2, (
            "seed='auto' should produce different auto-generated seeds on consecutive runs"
        )

        # Verify actual raster data differs
        array_1 = raster2numpy("rand_map_start_auto_seed")
        array_2 = raster2numpy("rand_map_start_auto_seed_2")
        assert not np.array_equal(array_1, array_2, equal_nan=True), (
            "seed='auto' should enable auto-seeding, producing different results each time"
        )

    def test_mapcalc_start_rand_with_explicit_seed_reproducible(self):
        """Test that explicit seed in mapcalc_start produces reproducible results.

        Verifies:
        1. Same seed produces identical raster arrays (byte-for-byte reproducibility)
        2. Different seeds produce different results (seed actually has effect)
        3. Seed metadata is correctly recorded for all variations
        """
        # Create two maps with same seed=12345
        p = gs.mapcalc_start(
            "rand_map_start_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        raster_info_1 = gs.raster_info("rand_map_start_seed", env=self.session.env)
        assert raster_info_1 is not None
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)
        assert seed_1 == 12345, f"Expected seed=12345 in metadata, got seed={seed_1}"

        # Read actual raster data
        array_1 = raster2numpy("rand_map_start_seed")
        assert array_1 is not None
        assert array_1.size > 0

        # Create another map with the same seed
        p = gs.mapcalc_start(
            "rand_map_start_seed_2 = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        raster_info_2 = gs.raster_info("rand_map_start_seed_2", env=self.session.env)
        assert raster_info_2 is not None
        comments_2 = raster_info_2.get("comments", "")
        seed_2 = extract_seed_from_comments(comments_2)
        assert seed_2 == 12345, f"Expected seed=12345 in metadata, got seed={seed_2}"

        # Read actual raster data
        array_2 = raster2numpy("rand_map_start_seed_2")
        assert array_2 is not None
        assert array_2.size > 0

        # Verify reproducibility: arrays must be identical (byte-for-byte)
        assert np.array_equal(array_1, array_2, equal_nan=True), (
            "Maps with same seed should have identical cell values"
        )

        # Verify seed actually has an effect by comparing with different seed
        p = gs.mapcalc_start(
            "rand_map_start_seed_3 = rand(0.0, 1.0)",
            seed=54321,
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        raster_info_3 = gs.raster_info("rand_map_start_seed_3", env=self.session.env)
        comments_3 = raster_info_3.get("comments", "")
        seed_3 = extract_seed_from_comments(comments_3)
        assert seed_3 == 54321, f"Expected seed=54321 in metadata, got seed={seed_3}"

        array_3 = raster2numpy("rand_map_start_seed_3")
        assert array_3 is not None

        # Different seed should produce different results
        assert not np.array_equal(array_1, array_3, equal_nan=True), (
            "Maps with different seeds should have different cell values"
        )
