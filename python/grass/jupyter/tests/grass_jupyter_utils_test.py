"""Tests for grass.jupyter.utils module

This module tests utility functions used throughout grass.jupyter.
Tests cover region management, coordinate transformation, querying and rendering size calculations.
"""

import os
import pytest
import grass.script as gs
from grass.tools import Tools

from grass.jupyter.utils import (
    get_region,
    reproject_region,
    get_location_proj_string,
    get_rendering_size,
    get_number_of_cores,
)


@pytest.fixture(scope="module")
def session_projected(tmp_path_factory):
    """Fixture providing a projected (EPSG:26917) GRASS session."""
    project = tmp_path_factory.mktemp("projected")
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_proj(flags="c", epsg=26917)
        tools.g_region(s=0, n=5, w=0, e=2, res=1)
        yield session


class TestGetRegion:
    """Tests for get_region function.
    This function returns the current computational region as a dictionary. It is used in every rendering operation throughout grass.jupyter.
    """

    def test_get_region_structure_and_values(self, session_with_data):
        """Test that get_region returns a correct region dictionary."""
        region = get_region(env=session_with_data.env)

        # Test it returns a dictionary
        assert isinstance(region, dict)

        # Test required keys exist
        required_keys = [
            "north",
            "south",
            "east",
            "west",
            "rows",
            "cols",
            "nsres",
            "ewres",
        ]
        for key in required_keys:
            assert key in region, f"Region missing required key: {key}"

        # Test coordinate boundaries
        assert region["north"] > region["south"], (
            "North coordinate should be greater than south"
        )
        assert region["east"] > region["west"], (
            "East coordinate should be greater than west"
        )

        # Test positive resolution and dimensions
        assert region["nsres"] > 0
        assert region["ewres"] > 0
        assert region["rows"] > 0
        assert region["cols"] > 0
        assert isinstance(region["rows"], int)
        assert isinstance(region["cols"], int)


class TestGetLocationProjString:
    """Tests for get_location_proj_string function.
    This function returns the projection of the current location
    in PROJ.4 format. Used in all reprojection operations."""

    def test_xy_returns_unprojected_string(self, session_with_data):
        """Test that an XY location returns an unprojected string indication."""
        proj_string = get_location_proj_string(env=session_with_data.env)

        assert isinstance(proj_string, str), (
            "get_location_proj_string should return a string"
        )
        assert len(proj_string) > 0, "PROJ string should not be empty"
        assert "XY location" in proj_string or "unprojected" in proj_string, (
            "XY location should be identified"
        )

    def test_projected_contains_proj_keyword(self, session_projected):
        """Test that a projected location returns a PROJ.4 format string."""
        proj_string = get_location_proj_string(env=session_projected.env)

        assert "+proj=" in proj_string, "Projected PROJ.4 string should contain +proj="


class TestGetRenderingSize:
    """Tests for get_rendering_size function.
    This function calculates appropriate rendering width and height based on region aspect ration. Used in all Map classes."""

    def test_both_dimensions_provided(self):
        """When both dimensions are given, they should be returned unchanged"""
        region = {"n": 100, "s": 0, "e": 200, "w": 0}
        width, height = get_rendering_size(region, 800, 600, 600, 400)
        assert width == 800
        assert height == 600

    def test_only_width_provided_landscape(self):
        """Test aspect ratio calculation when only width provided(landscape)."""
        region = {"n": 100, "s": 0, "e": 200, "w": 0}
        width, height = get_rendering_size(region, 800, None, 600, 400)
        assert width == 800
        assert height == 400

    def test_only_width_provided_portrait(self):
        """Test aspect ratio calculation when only with provided(portrait)"""
        region = {"n": 200, "s": 0, "e": 100, "w": 0}
        width, height = get_rendering_size(region, 400, None, 600, 400)
        assert width == 400
        assert height == 800

    def test_only_height_provided_landscape(self):
        """Test aspect ration calculation when only height provided(landscape)"""
        region = {"n": 100, "s": 0, "e": 200, "w": 0}
        width, height = get_rendering_size(region, None, 400, 600, 400)
        assert height == 400
        assert width == 800

    def test_only_height_provided_portrait(self):
        """Test aspect ratio calculation when only height provided(portrait)"""
        region = {"n": 200, "s": 0, "e": 100, "w": 0}
        width, height = get_rendering_size(region, None, 800, 600, 400)
        assert height == 800
        assert width == 400

    def test_no_dimensions_provided_square(self):
        """Test default dimensions when neither provided (square region)"""
        region = {"n": 100, "s": 0, "e": 100, "w": 0}
        width, height = get_rendering_size(region, None, None, 600, 400)
        # For square region, function uses default width and adjusts height
        assert width == 600
        assert height == 600  # Should match width for 1:1 aspect ratio

    def test_no_dimensions_provided_landscape(self):
        """Test default dimensions when neither provided (landscape region)"""
        region = {"n": 100, "s": 0, "e": 200, "w": 0}
        width, height = get_rendering_size(region, None, None, 600, 400)
        assert width == 600
        assert height == 300

    def test_no_dimensions_provided_portrait(self):
        """Test default dimensions when neither provided (portrait region)"""
        region = {"n": 200, "s": 0, "e": 100, "w": 0}
        width, height = get_rendering_size(region, None, None, 600, 400)
        assert height == 400
        assert width == 200


class TestGetNumberOfCores:
    """Tests for get_number_of_cores function.
    This function determines how many CPU cores to use for
    parallel operations.
    """

    def _clear_nprocs(self, env):
        try:
            gs.run_command("g.gisenv", unset="NPROCS", env=env)
        except Exception:
            pass

    def test_returns_positive_integer(self, session_with_data):
        """Test that function returns a positive integer."""
        self._clear_nprocs(session_with_data.env)
        cores = get_number_of_cores(4, env=session_with_data.env)
        assert isinstance(cores, int), "Should return an integer"
        assert cores > 0, "Should return at least 1 core"

    def test_respects_requested_cores(self, session_with_data):
        """Test that function respects reasonable requested core count."""
        self._clear_nprocs(session_with_data.env)
        cores = get_number_of_cores(2, env=session_with_data.env)
        assert cores >= 1, "Should return at least 1 core"
        assert cores <= 2, "Should not exceed requested cores"

    def test_returns_zero_on_zero_request(self, session_with_data):
        """Test that function handles zero requested cores by returning zero.

        Note: The function returns 0 when requested is 0, which is
        the actual documented behavior of the upstream function.
        """
        self._clear_nprocs(session_with_data.env)
        cores = get_number_of_cores(0, env=session_with_data.env)
        assert cores == 0, "Should return exactly 0 when requested is 0"

    def test_caps_at_available_cores(self, session_with_data):
        """Test that function doesn't exceed available system cores."""
        import multiprocessing

        self._clear_nprocs(session_with_data.env)
        max_cores = multiprocessing.cpu_count()
        cores = get_number_of_cores(9999, env=session_with_data.env)
        assert cores <= max_cores, "Should not exceed available system cores"

    def test_respects_nprocs_env_var(self, session_with_data):
        """Test that NPROCS variable bypasses other logic."""
        gs.run_command("g.gisenv", set="NPROCS=42", env=session_with_data.env)
        try:
            cores = get_number_of_cores(2, env=session_with_data.env)
            assert cores == 42, "Given NPROCS is checked first, it should return 42"
        finally:
            self._clear_nprocs(session_with_data.env)


class TestReprojectRegion:
    """Tests for reproject_region function.

    This function reprojects region boundaries from one projection
    to another. Critical for InteractiveMap coordinate transformation.
    """

    def test_returns_dict(self, session_projected):
        """Test that function returns a dictionary."""
        region = get_region(env=session_projected.env)
        from_proj = get_location_proj_string(env=session_projected.env)
        to_proj = "+proj=longlat +datum=WGS84 +no_defs"

        reprojected = reproject_region(region, from_proj, to_proj)
        assert isinstance(reprojected, dict), "reproject_region should return a dict"

    def test_has_same_keys(self, session_projected):
        """Test that reprojected region has same boundary keys as input."""
        region = get_region(env=session_projected.env)
        from_proj = get_location_proj_string(env=session_projected.env)
        to_proj = "+proj=longlat +datum=WGS84 +no_defs"

        reprojected = reproject_region(region, from_proj, to_proj)

        # Should have at least the boundary keys
        assert "north" in reprojected
        assert "south" in reprojected
        assert "east" in reprojected
        assert "west" in reprojected

    def test_north_still_greater_than_south(self, session_projected):
        """Test that coordinate relationships are preserved after reprojection."""
        region = get_region(env=session_projected.env)
        from_proj = get_location_proj_string(env=session_projected.env)
        to_proj = "+proj=longlat +datum=WGS84 +no_defs"

        reprojected = reproject_region(region, from_proj, to_proj)
        assert reprojected["north"] > reprojected["south"], (
            "North should still be greater than south after reprojection"
        )

    def test_coordinates_changed(self, session_projected):
        """Test that coordinates actually change during reprojection."""
        region = get_region(env=session_projected.env)
        from_proj = get_location_proj_string(env=session_projected.env)
        to_proj = "+proj=longlat +datum=WGS84 +no_defs"

        # Only test if projections are different
        if from_proj != to_proj:
            reprojected = reproject_region(region, from_proj, to_proj)
            # At least one coordinate should change
            changed = (
                reprojected["north"] != region["north"]
                or reprojected["south"] != region["south"]
                or reprojected["east"] != region["east"]
                or reprojected["west"] != region["west"]
            )
            assert changed, "Coordinates should change during reprojection"
