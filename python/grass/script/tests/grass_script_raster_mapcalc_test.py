"""Tests for grass.script.raster mapcalc functions"""

import pytest

import grass.script as gs


class TestMapcalcRandFunction:
    """Tests for rand() function in mapcalc with automatic seeding

    r.mapcalc now auto-seeds by default when no explicit seed is provided.
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
            name="rand_map,rand_map_seed,rand_map_no_flags",
            env=self.session.env,
            quiet=True,
            errors="ignore",
        )

    def test_mapcalc_rand_autoseeded(self):
        """Test that rand() works with automatic seeding (no parameters needed)"""
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

    def test_mapcalc_rand_with_explicit_flags_s(self):
        """Test that rand() works when explicitly using flags='s' (deprecated but still supported)"""
        gs.mapcalc(
            "rand_map_no_flags = rand(0.0, 1.0)",
            flags="s",
            env=self.session.env,
        )
        # Check that the map was created successfully
        raster_info = gs.raster_info("rand_map_no_flags", env=self.session.env)
        assert raster_info is not None
        assert "min" in raster_info

    def test_mapcalc_rand_with_seed_auto_deprecated(self):
        """Test that seed='auto' (deprecated) still works via graceful handling"""
        # seed="auto" is deprecated but should still work
        # Python converts it to None, r.mapcalc auto-seeds
        gs.mapcalc(
            "rand_map_seed = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        # Check that the map was created successfully
        raster_info = gs.raster_info("rand_map_seed", env=self.session.env)
        assert raster_info is not None
        assert "min" in raster_info

    def test_mapcalc_rand_flags_s_and_seed_are_mutually_exclusive(self):
        """Test that flags='s' and seed= are mutually exclusive"""
        from grass.script.core import CalledModuleError

        with pytest.raises(CalledModuleError):
            gs.mapcalc(
                "rand_map_error = rand(0.0, 1.0)",
                flags="s",
                seed=12345,
                env=self.session.env,
            )


class TestMapcalcStartRandFunction:
    """Tests for rand() function in mapcalc_start with automatic seeding

    r.mapcalc now auto-seeds by default when no explicit seed is provided.
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
            name="rand_map_start,rand_map_start_seed,rand_map_start_no_flags",
            env=self.session.env,
            quiet=True,
            errors="ignore",
        )

    def test_mapcalc_start_rand_autoseeded(self):
        """Test that mapcalc_start with rand() works with automatic seeding (no parameters needed)"""
        p = gs.mapcalc_start(
            "rand_map_start = rand(0.0, 1.0)",
            env=self.session.env,
        )
        returncode = p.wait()
        # Should succeed - r.mapcalc now auto-seeds by default
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

    def test_mapcalc_start_rand_with_explicit_flags_s(self):
        """Test that mapcalc_start with rand() works when explicitly using flags='s' (deprecated but still supported)"""
        p = gs.mapcalc_start(
            "rand_map_start_no_flags = rand(0.0, 1.0)",
            flags="s",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0

    def test_mapcalc_start_rand_with_seed_auto_deprecated(self):
        """Test that seed='auto' (deprecated) still works via graceful handling in mapcalc_start"""
        # seed="auto" is deprecated but should still work
        # Python converts it to None, r.mapcalc auto-seeds
        p = gs.mapcalc_start(
            "rand_map_start_seed = rand(0.0, 1.0)",
            seed="auto",
            env=self.session.env,
        )
        returncode = p.wait()
        assert returncode == 0

    def test_mapcalc_start_rand_flags_s_and_seed_are_mutually_exclusive(self):
        """Test that flags='s' and seed= are mutually exclusive in mapcalc_start

        According to r.mapcalc CLI, -s flag and seed= option cannot be used together.
        This test verifies that attempting to use both results in a non-zero return code.
        """
        p = gs.mapcalc_start(
            "rand_map_error = rand(0.0, 1.0)",
            flags="s",
            seed=12345,
            env=self.session.env,
        )
        returncode = p.wait()
        # Should fail because -s and seed= are mutually exclusive
        assert returncode != 0
