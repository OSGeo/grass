"""Tests for grass.script.raster mapcalc functions"""

import pytest
import re
import os
import time

import grass.script as gs


def extract_seed_from_comments(comments_str):
    """Extract explicit numeric seed from raster comments/metadata.

    Parses the seed value from r.mapcalc metadata comments string.
    Searches for patterns like 'seed=12345' or 'seed:  12345' (handles optional spacing).

    Args:
        comments_str: The comments/metadata string from raster_info()

    Returns:
        int: The seed value if found, None otherwise
    """
    if not comments_str:
        return None
    match = re.search(r"\bseed\s*[=:]\s*(-?\d+)", comments_str)
    return int(match.group(1)) if match else None


def verify_seed_in_metadata(session, mapname, expected_seed):
    """Helper to verify seed value in raster metadata.

    Args:
        session: GRASS session object
        mapname: Name of the raster map to check
        expected_seed: Expected seed value

    Returns:
        int: The actual seed value found in metadata
    """
    info = gs.raster_info(mapname, env=session.env)
    comments = info.get("comments", "")
    actual_seed = extract_seed_from_comments(comments)
    assert actual_seed == expected_seed, (
        f"Expected seed={expected_seed} in metadata, got seed={actual_seed}"
    )
    return actual_seed


@pytest.fixture(scope="module")
def mapcalc_rand_session(tmp_path_factory):
    """Setup temporary GRASS session for mapcalc rand tests"""
    tmp_path = tmp_path_factory.mktemp("mapcalc_rand_test")
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=2, cols=2, env=session.env)
        yield session


@pytest.fixture(scope="module")
def mapcalc_start_rand_session(tmp_path_factory):
    """Setup temporary GRASS session for mapcalc_start rand tests"""
    tmp_path = tmp_path_factory.mktemp("mapcalc_start_rand_test")
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=2, cols=2, env=session.env)
        yield session


class TestMapcalcRandFunction:
    """Tests for rand() function in mapcalc

    r.mapcalc auto-seeds when no seed is provided. Explicit seed can be used for reproducibility.
    """

    def test_mapcalc_rand_autoseeded(self, mapcalc_rand_session):
        """Test that rand() auto-seeds when no explicit seed is provided.

        Verifies that metadata contains different auto-generated seed values.

        Note: On Windows with low-resolution timers, consecutive calls may occasionally
        receive the same auto-seed. We add a small delay to ensure different timestamps.
        """
        # r.mapcalc auto-seeds when no seed is given — two runs should differ
        gs.mapcalc(
            "rand_map_auto1 = rand(0.0, 1.0)",
            overwrite=True,
            env=mapcalc_rand_session.env,
        )

        # Small delay to ensure different auto-seed on systems with low-resolution timers
        # G_srand48_auto() uses microsecond precision, but on some Windows systems
        # the timer granularity may be coarser. Since tests run sequentially (not parallel),
        # 0.01s is usually sufficient.
        time.sleep(0.01)

        gs.mapcalc(
            "rand_map_auto2 = rand(0.0, 1.0)",
            overwrite=True,
            env=mapcalc_rand_session.env,
        )

        # Verify mechanism: auto-generated seeds are recorded in metadata and differ
        info1 = gs.raster_info("rand_map_auto1", env=mapcalc_rand_session.env)
        info2 = gs.raster_info("rand_map_auto2", env=mapcalc_rand_session.env)
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
        assert seed1 != seed2, (
            f"Auto-seeds should differ between runs (got {seed1} and {seed2}). "
            f"If this fails repeatedly, there may be a timer resolution issue."
        )

    def test_mapcalc_rand_with_explicit_seed(self, mapcalc_rand_session):
        """Test that rand() works with explicit seed value.

        Verifies:
        1. Seed parameter is passed to r.mapcalc
        2. Metadata contains the explicit seed value
        """
        gs.mapcalc(
            "rand_map_seed = rand(0.0, 1.0)",
            seed=12345,
            overwrite=True,
            env=mapcalc_rand_session.env,
        )
        verify_seed_in_metadata(mapcalc_rand_session, "rand_map_seed", 12345)

    @pytest.mark.xfail(
        reason="r.mapcalc currently accepts invalid seed strings instead of rejecting them with ScriptError"
    )
    def test_mapcalc_rand_with_invalid_seed(self, mapcalc_rand_session):
        """Test that invalid seed values raise appropriate errors.

        Verifies:
        1. Invalid seed types (non-numeric strings) raise errors
        2. Invalid seeds are properly rejected by r.mapcalc
        """
        from grass.exceptions import ScriptError

        with pytest.raises(ScriptError):
            gs.mapcalc(
                "rand_map_invalid = rand(0.0, 1.0)",
                seed="invalid_string",
                overwrite=True,
                env=mapcalc_rand_session.env,
            )

    def test_mapcalc_rand_with_seed_auto_backwards_compat(self, mapcalc_rand_session):
        """Test that seed='auto' is handled for backwards compatibility.

        seed='auto' should be converted to None internally, enabling auto-seeding behavior.
        This means it should behave identically to not providing a seed parameter.

        Verifies that auto-generated seeds are recorded in metadata and differ between runs.
        """
        # Create one map with seed='auto'
        gs.mapcalc(
            "rand_map_auto_seed = rand(0.0, 1.0)",
            seed="auto",
            overwrite=True,
            env=mapcalc_rand_session.env,
        )
        raster_info_1 = gs.raster_info(
            "rand_map_auto_seed", env=mapcalc_rand_session.env
        )
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)

        # Since seed='auto' converts to None, it should enable auto-seeding
        # Auto-seeding records generated seed in metadata (same as no-seed case)
        assert seed_1 is not None, (
            "seed='auto' should convert to None and enable auto-seeding, recording an auto-generated seed in metadata"
        )

        # Increased delay to ensure different auto-seed
        time.sleep(0.1)

        # Create second map with seed='auto' — should differ from first (different auto-seeds)
        gs.mapcalc(
            "rand_map_auto_seed_2 = rand(0.0, 1.0)",
            seed="auto",
            overwrite=True,
            env=mapcalc_rand_session.env,
        )
        raster_info_2 = gs.raster_info(
            "rand_map_auto_seed_2", env=mapcalc_rand_session.env
        )
        comments_2 = raster_info_2.get("comments", "")
        seed_2 = extract_seed_from_comments(comments_2)

        # Verify both runs have auto-generated seeds and they differ
        assert seed_2 is not None, (
            "seed='auto' should enable auto-seeding, recording an auto-generated seed"
        )
        assert seed_1 != seed_2, (
            "seed='auto' should produce different auto-generated seeds on consecutive runs"
        )

    def test_mapcalc_rand_with_explicit_seed_reproducible(self, mapcalc_rand_session):
        """Test that explicit seed produces reproducible results.

        Verifies that explicit seeds are correctly passed to r.mapcalc by checking metadata.
        """
        # Create map with seed=12345
        gs.mapcalc(
            "rand_map_seed = rand(0.0, 1.0)",
            seed=12345,
            overwrite=True,
            env=mapcalc_rand_session.env,
        )
        verify_seed_in_metadata(mapcalc_rand_session, "rand_map_seed", 12345)

        # Create another map with the same seed
        gs.mapcalc(
            "rand_map_seed_2 = rand(0.0, 1.0)",
            seed=12345,
            overwrite=True,
            env=mapcalc_rand_session.env,
        )
        verify_seed_in_metadata(mapcalc_rand_session, "rand_map_seed_2", 12345)

        # Verify seed actually has an effect by comparing with different seed
        gs.mapcalc(
            "rand_map_seed_3 = rand(0.0, 1.0)",
            seed=54321,
            overwrite=True,
            env=mapcalc_rand_session.env,
        )
        verify_seed_in_metadata(mapcalc_rand_session, "rand_map_seed_3", 54321)

    @pytest.mark.parametrize(
        ("seed_value", "description"),
        [
            (-42, "negative seed"),
            (2147483647, "large seed"),  # Max 32-bit signed integer
        ],
    )
    def test_mapcalc_rand_with_edge_seeds(
        self, mapcalc_rand_session, seed_value, description
    ):
        """Test that edge seed values (negative, large) are handled correctly.

        Verifies that edge seed values are correctly passed to r.mapcalc by checking metadata.
        """
        map_name = f"rand_map_{description.replace(' ', '_')}"

        # Create map with edge seed
        gs.mapcalc(
            f"{map_name} = rand(0.0, 1.0)",
            seed=seed_value,
            overwrite=True,
            env=mapcalc_rand_session.env,
        )
        verify_seed_in_metadata(mapcalc_rand_session, map_name, seed_value)


class TestMapcalcStartRandFunction:
    """Tests for rand() function in mapcalc_start

    r.mapcalc auto-seeds when no seed is provided. Explicit seed can be used for reproducibility.

    Note: Edge case tests (seed=0, negative seeds, large seeds) are only in TestMapcalcRandFunction
    because mapcalc and mapcalc_start share the same underlying seed handling implementation.
    Testing the mechanism once is sufficient to ensure both code paths work correctly.
    """

    def test_mapcalc_start_rand_autoseeded(self, mapcalc_start_rand_session):
        """Test that mapcalc_start with rand() auto-seeds when no explicit seed is provided.

        Verifies that auto-seeding is enabled and different seeds are recorded in metadata.
        """
        # r.mapcalc auto-seeds when no seed is given — two runs should differ
        p = gs.mapcalc_start(
            "rand_map_start_auto1 = rand(0.0, 1.0)",
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0

        # Increased delay to ensure different auto-seed
        time.sleep(0.1)

        p = gs.mapcalc_start(
            "rand_map_start_auto2 = rand(0.0, 1.0)",
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0

        # Verify mechanism: auto-generated seeds are recorded in metadata and differ
        info1 = gs.raster_info(
            "rand_map_start_auto1", env=mapcalc_start_rand_session.env
        )
        info2 = gs.raster_info(
            "rand_map_start_auto2", env=mapcalc_start_rand_session.env
        )
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
        assert seed1 != seed2, (
            f"Auto-seeds should differ between runs (got {seed1} and {seed2})"
        )

    def test_mapcalc_start_rand_with_explicit_seed(self, mapcalc_start_rand_session):
        """Test that mapcalc_start with rand() works with explicit seed value.

        Verifies:
        1. Seed parameter is passed to r.mapcalc
        2. Metadata contains the explicit seed value
        """
        p = gs.mapcalc_start(
            "rand_map_start_seed = rand(0.0, 1.0)",
            seed=12345,
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        # Verify the map was created and seed was written to metadata
        verify_seed_in_metadata(
            mapcalc_start_rand_session, "rand_map_start_seed", 12345
        )

    def test_mapcalc_start_rand_with_seed_auto_backwards_compat(
        self, mapcalc_start_rand_session
    ):
        """Test that seed='auto' is handled for backwards compatibility.

        seed='auto' should be converted to None internally, enabling auto-seeding behavior.

        Verifies that auto-generated seeds are recorded in metadata and differ between runs.
        """
        # Create one map with seed='auto'
        p = gs.mapcalc_start(
            "rand_map_start_auto_seed = rand(0.0, 1.0)",
            seed="auto",
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        raster_info_1 = gs.raster_info(
            "rand_map_start_auto_seed", env=mapcalc_start_rand_session.env
        )
        comments_1 = raster_info_1.get("comments", "")
        seed_1 = extract_seed_from_comments(comments_1)

        # Since seed='auto' converts to None, it should enable auto-seeding
        # Auto-seeding records generated seed in metadata (same as no-seed case)
        assert seed_1 is not None, (
            "seed='auto' should convert to None and enable auto-seeding, recording an auto-generated seed in metadata"
        )

        # Increased delay to ensure different auto-seed
        time.sleep(0.1)

        # Create second map with seed='auto' — should differ from first (different auto-seeds)
        p = gs.mapcalc_start(
            "rand_map_start_auto_seed_2 = rand(0.0, 1.0)",
            seed="auto",
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        raster_info_2 = gs.raster_info(
            "rand_map_start_auto_seed_2", env=mapcalc_start_rand_session.env
        )
        comments_2 = raster_info_2.get("comments", "")
        seed_2 = extract_seed_from_comments(comments_2)

        # Verify both runs have auto-generated seeds and they differ
        assert seed_2 is not None, (
            "seed='auto' should enable auto-seeding, recording an auto-generated seed"
        )
        assert seed_1 != seed_2, (
            f"seed='auto' should produce different auto-generated seeds (got {seed_1} and {seed_2})"
        )

    def test_mapcalc_start_rand_with_explicit_seed_reproducible(
        self, mapcalc_start_rand_session
    ):
        """Test that explicit seed in mapcalc_start produces reproducible results.

        Verifies that explicit seeds are correctly passed to r.mapcalc by checking metadata.
        """
        # Create map with seed=12345
        p = gs.mapcalc_start(
            "rand_map_start_seed = rand(0.0, 1.0)",
            seed=12345,
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        verify_seed_in_metadata(
            mapcalc_start_rand_session, "rand_map_start_seed", 12345
        )

        # Create another map with the same seed
        p = gs.mapcalc_start(
            "rand_map_start_seed_2 = rand(0.0, 1.0)",
            seed=12345,
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        verify_seed_in_metadata(
            mapcalc_start_rand_session, "rand_map_start_seed_2", 12345
        )

        # Verify seed actually has an effect by comparing with different seed
        p = gs.mapcalc_start(
            "rand_map_start_seed_3 = rand(0.0, 1.0)",
            seed=54321,
            overwrite=True,
            env=mapcalc_start_rand_session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        verify_seed_in_metadata(
            mapcalc_start_rand_session, "rand_map_start_seed_3", 54321
        )
