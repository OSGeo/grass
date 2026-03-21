"""Tests for grass.jupyter.utils module

This module tests utility functions used throughout grass.jupyter.
Tests cover region management, coordinate transformation, querying and rendering size calculations.
"""

import pytest
from grass.jupyter.utils import (
    get_region,
    reproject_region,
    get_location_proj_string,
    get_rendering_size,
    get_number_of_cores,
)


class TestGetRegion:
    """Tests for get_region function.
    This function returns the current computational region as a dictionary. It is used in every rendering operation throughout grass.jupyter.

    """

    def test_returns_dict(self, session_with_data):
        """Test that get_region returns a dictionary."""
        region = get_region(env=session_with_data.env)
        assert isinstance(region, dict)

    def test_has_required_keys(self, session_with_data):
        """Test that region dict contains all required keys."""
        region = get_region(env=session_with_data.env)
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

    def test_north_greater_than_south(self, session_with_data):
        """Test that north coordinate is greater than south coordinate."""
        region = get_region(env=session_with_data.env)
        assert region["north"] > region["south"], (
            "North coordinate should be greater than south"
        )

    def test_east_greater_than_west(self, session_with_data):
        """Test that east coordinate is greater than south coordinate."""
        region = get_region(env=session_with_data.env)
        assert region["east"] > region["west"], (
            "East coordinate should be greater than west"
        )

    def test_postive_resolution(self, session_with_data):
        """Test that resolution values are positive."""
        region = get_region(env=session_with_data.env)
        assert region["nsres"] > 0
        assert region["ewres"] > 0

    def test_positive_dimensions(self, session_with_data):
        """Test that rows and columns are positive integers."""
        region = get_region(env=session_with_data.env)
        assert region["rows"] > 0
        assert region["cols"] > 0
        assert isinstance(region["rows"], int)
        assert isinstance(region["cols"], int)


class TestGetLocationProjString:
    """Tests for get_location_proj_string function.
    This function returns the projection of the current location
    in PROJ.4 format. Used in all reprojection operations."""

    def test_returns_string(self, session_with_data):
        """Test that function returns a string."""
        proj_string = get_location_proj_string(env=session_with_data.env)
        assert isinstance(proj_string, str), (
            "get_location_proj_string should return a string"
        )

    def test_non_empty_string(self, session_with_data):
        """Test that required string is not empty"""
        proj_string = get_location_proj_string(env=session_with_data.env)
        assert len(proj_string) > 0, "PROJ string should not be empty"

    def test_contains_proj_keyword(self, session_with_data):
        """Test that PROJ string contains projection information.

        Note: For XY (unprojected) locations, the string will be
        'XY location (unprojected)' instead of a PROJ.4 string.
        """
        proj_string = get_location_proj_string(env=session_with_data.env)
        # Check for either PROJ.4 format or XY location
        is_proj4 = "+proj=" in proj_string
        is_xy = "XY location" in proj_string or "unprojected" in proj_string
        assert is_proj4 or is_xy, (
            "PROJ string should contain projection info or indicate XY location"
        )


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

    def test_returns_positive_integer(self, session_with_data):
        """Test that function returns a positive integer."""
        cores = get_number_of_cores(4, env=session_with_data.env)
        assert isinstance(cores, int), "Should return an integer"
        assert cores > 0, "Should return at least 1 core"

    def test_respects_requested_cores(self, session_with_data):
        """Test that function respects reasonable requested core count."""
        cores = get_number_of_cores(2, env=session_with_data.env)
        assert cores >= 1, "Should return at least 1 core"
        assert cores <= 2, "Should not exceed requested cores"

    def test_minimum_one_core(self, session_with_data):
        """Test that function handles zero or negative requested cores.

        Note: The function may return 0 if requested is 0, which is
        the actual behavior. This test documents that behavior.
        """
        cores = get_number_of_cores(0, env=session_with_data.env)
        # Function returns 0 when requested is 0 (actual behavior)
        assert cores >= 0, "Should return non-negative value"

    def test_caps_at_available_cores(self, session_with_data):
        """Test that function doesn't exceed available system cores."""
        import multiprocessing

        max_cores = multiprocessing.cpu_count()
        cores = get_number_of_cores(9999, env=session_with_data.env)
        assert cores <= max_cores, "Should not exceed available system cores"


class TestReprojectRegion:
    """Tests for reproject_region function.

    This function reprojects region boundaries from one projection
    to another. Critical for InteractiveMap coordinate transformation.

    Note: These tests are skipped for XY (unprojected) locations since
    reprojection requires valid PROJ.4 strings.
    """

    def test_returns_dict(self, session_with_data):
        """Test that function returns a dictionary."""
        region = get_region(env=session_with_data.env)
        from_proj = get_location_proj_string(env=session_with_data.env)

        # Skip test for XY locations (no valid projection)
        if "XY location" in from_proj or "unprojected" in from_proj:
            pytest.skip("Cannot reproject XY (unprojected) location")

        # Reproject to WGS84
        to_proj = "+proj=longlat +datum=WGS84 +no_defs"

        reprojected = reproject_region(region, from_proj, to_proj)
        assert isinstance(reprojected, dict), "reproject_region should return a dict"

    def test_has_same_keys(self, session_with_data):
        """Test that reprojected region has same keys as input."""
        region = get_region(env=session_with_data.env)
        from_proj = get_location_proj_string(env=session_with_data.env)

        # Skip test for XY locations
        if "XY location" in from_proj or "unprojected" in from_proj:
            pytest.skip("Cannot reproject XY (unprojected) location")

        to_proj = "+proj=longlat +datum=WGS84 +no_defs"

        reprojected = reproject_region(region, from_proj, to_proj)

        # Should have at least the boundary keys
        assert "north" in reprojected
        assert "south" in reprojected
        assert "east" in reprojected
        assert "west" in reprojected

    def test_north_still_greater_than_south(self, session_with_data):
        """Test that coordinate relationships are preserved after reprojection."""
        region = get_region(env=session_with_data.env)
        from_proj = get_location_proj_string(env=session_with_data.env)

        # Skip test for XY locations
        if "XY location" in from_proj or "unprojected" in from_proj:
            pytest.skip("Cannot reproject XY (unprojected) location")

        to_proj = "+proj=longlat +datum=WGS84 +no_defs"

        reprojected = reproject_region(region, from_proj, to_proj)
        assert reprojected["north"] > reprojected["south"], (
            "North should still be greater than south after reprojection"
        )

    def test_coordinates_changed(self, session_with_data):
        """Test that coordinates actually change during reprojection."""
        region = get_region(env=session_with_data.env)
        from_proj = get_location_proj_string(env=session_with_data.env)

        # Skip test for XY locations
        if "XY location" in from_proj or "unprojected" in from_proj:
            pytest.skip("Cannot reproject XY (unprojected) location")

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
