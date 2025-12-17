"""Tests for grass.script.raster mapcalc functions"""

import pytest

import grass.script as gs


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
        """Clean up maps after each test"""
        gs.run_command(
            "g.remove",
            type="raster",
            flags="f",
            name="rand_map,rand_map_seed,rand_map_auto_seed",
            env=self.session.env,
            quiet=True,
            errors="ignore",
        )

    def test_mapcalc_rand_autoseeded(self):
        """Test that rand() auto-seeds when no explicit seed is provided"""
        # r.mapcalc auto-seeds when no seed is given
        gs.mapcalc(
            "rand_map = rand(0.0, 1.0)",
            env=self.session.env,
        )
        # Check that the map was created successfully
        raster_info = gs.raster_info("rand_map", env=self.session.env)
        assert raster_info is not None
        assert "min" in raster_info

    def test_mapcalc_rand_with_explicit_seed(self):
        """Test that rand() works with explicit seed value"""
        gs.mapcalc(
            "rand_map_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        # Check that the map was created successfully
        raster_info = gs.raster_info("rand_map_seed", env=self.session.env)
        assert raster_info is not None
        assert "min" in raster_info

    def test_mapcalc_rand_with_seed_auto_deprecated(self):
        """Test that seed='auto' is handled properly (converted to None, C auto-seeds)"""
        # seed="auto" is deprecated; Python converts it to None
        # r.mapcalc will auto-seed in this case
        gs.mapcalc(
            "rand_map_auto_seed = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        # Check that the map was created successfully with auto-seeding
        raster_info = gs.raster_info("rand_map_auto_seed", env=self.session.env)
        assert raster_info is not None
        assert "min" in raster_info

    def test_mapcalc_rand_with_explicit_seed_reproducible(self):
        """Test that explicit seed produces reproducible results"""
        # Create two maps with same seed
        gs.mapcalc(
            "rand_map_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        raster_info = gs.raster_info("rand_map_seed", env=self.session.env)
        assert raster_info is not None
        assert "min" in raster_info


class TestMapcalcStartRandFunction:
    """Tests for rand() function in mapcalc_start

    r.mapcalc auto-seeds when no seed is provided. Explicit seed can be used for reproducibility.
    """

    @pytest.fixture(autouse=True)
    def setup_region(self, session_2x2):
        """Setup region for tests"""
        self.session = session_2x2
        gs.run_command("g.region", rows=10, cols=10, env=self.session.env)

    def teardown_method(self):
        """Clean up maps after each test"""
        gs.run_command(
            "g.remove",
            type="raster",
            flags="f",
            name="rand_map_start,rand_map_start_seed,rand_map_start_auto_seed",
            env=self.session.env,
            quiet=True,
            errors="ignore",
        )

    def test_mapcalc_start_rand_autoseeded(self):
        """Test that mapcalc_start with rand() auto-seeds when no explicit seed is provided"""
        # r.mapcalc auto-seeds when no seed is given
        p = gs.mapcalc_start(
            "rand_map_start = rand(0.0, 1.0)",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0

    def test_mapcalc_start_rand_with_explicit_seed(self):
        """Test that mapcalc_start with rand() works with explicit seed value"""
        p = gs.mapcalc_start(
            "rand_map_start_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0

    def test_mapcalc_start_rand_with_seed_auto_deprecated(self):
        """Test that seed='auto' is handled properly in mapcalc_start (converted to None, C auto-seeds)"""
        # seed="auto" is deprecated; Python converts it to None
        # r.mapcalc will auto-seed in this case
        p = gs.mapcalc_start(
            "rand_map_start_auto_seed = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0

    def test_mapcalc_start_rand_with_explicit_seed_reproducible(self):
        """Test that explicit seed in mapcalc_start produces reproducible results"""
        # Create map with explicit seed
        p = gs.mapcalc_start(
            "rand_map_start_seed = rand(0.0, 1.0)",
            seed=12345,
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0
        # Verify the map was created successfully with the seed value
        raster_info = gs.raster_info("rand_map_start_seed", env=self.session.env)
        assert raster_info is not None
        assert "min" in raster_info
