"""Test utility functions for grass.jupyter

These tests cover pure Python utility functions in grass.jupyter.utils
that don't require an active GRASS session.
"""

import pytest

from grass.jupyter.utils import (
    get_rendering_size,
    get_map_name_from_d_command,
    _style_table,
    _format_nested_table,
    _format_regular_output,
)


class TestGetRenderingSize:
    """Tests for get_rendering_size() function."""

    def test_both_dimensions_provided(self):
        """When both width and height provided, return them as-is without adjustment."""
        region = {"n": 100, "s": 0, "e": 100, "w": 0}
        width, height = get_rendering_size(region, width=800, height=600)
        assert width == 800
        assert height == 600

    def test_width_only_square_region(self):
        """When only width provided with square region, height equals width."""
        region = {"n": 100, "s": 0, "e": 100, "w": 0}
        width, height = get_rendering_size(region, width=800, height=None)
        assert width == 800
        assert height == 800  # Square region, same aspect ratio

    def test_width_only_wide_region(self):
        """When only width provided with wide region, height is smaller."""
        region = {"n": 50, "s": 0, "e": 100, "w": 0}  # 2:1 aspect ratio
        width, height = get_rendering_size(region, width=800, height=None)
        assert width == 800
        assert height == 400  # Half the width due to 2:1 ratio

    def test_width_only_tall_region(self):
        """When only width provided with tall region, height is larger."""
        region = {"n": 100, "s": 0, "e": 50, "w": 0}  # 1:2 aspect ratio
        width, height = get_rendering_size(region, width=400, height=None)
        assert width == 400
        assert height == 800  # Double the width due to 1:2 ratio

    def test_height_only_square_region(self):
        """When only height provided with square region, width equals height."""
        region = {"n": 100, "s": 0, "e": 100, "w": 0}
        width, height = get_rendering_size(region, width=None, height=600)
        assert width == 600
        assert height == 600

    def test_height_only_wide_region(self):
        """When only height provided with wide region, width is larger."""
        region = {"n": 50, "s": 0, "e": 100, "w": 0}  # 2:1 aspect ratio
        width, height = get_rendering_size(region, width=None, height=400)
        assert width == 800  # Double the height due to 2:1 ratio
        assert height == 400

    def test_neither_dimension_defaults_wide_region(self):
        """When neither provided with wide region, use default width."""
        region = {"n": 50, "s": 0, "e": 100, "w": 0}  # 2:1 aspect ratio
        width, height = get_rendering_size(region, width=None, height=None)
        assert width == 600  # default_width
        assert height == 300  # Computed from aspect ratio

    def test_neither_dimension_defaults_tall_region(self):
        """When neither provided with tall region, use default height."""
        region = {"n": 100, "s": 0, "e": 50, "w": 0}  # 1:2 aspect ratio (tall)
        width, height = get_rendering_size(region, width=None, height=None)
        # For tall regions, default_height is used
        assert height == 400  # default_height
        assert width == 200  # Computed from aspect ratio

    def test_custom_defaults(self):
        """Test with custom default dimensions."""
        region = {"n": 100, "s": 0, "e": 100, "w": 0}  # Square
        width, height = get_rendering_size(
            region, width=None, height=None, default_width=1200, default_height=800
        )
        assert width == 1200
        assert height == 1200


class TestGetMapNameFromDCommand:
    """Tests for get_map_name_from_d_command() function."""

    def test_d_rast_uses_map_parameter(self):
        """d.rast should extract from 'map' parameter."""
        result = get_map_name_from_d_command("d.rast", map="elevation")
        assert result == "elevation"

    def test_d_vect_uses_map_parameter(self):
        """d.vect should extract from 'map' parameter."""
        result = get_map_name_from_d_command("d.vect", map="roads")
        assert result == "roads"

    def test_d_legend_uses_raster_parameter(self):
        """d.legend is special case using 'raster' parameter."""
        result = get_map_name_from_d_command("d.legend", raster="elevation")
        assert result == "elevation"

    def test_d_shade_uses_shade_parameter(self):
        """d.shade is special case using 'shade' parameter."""
        result = get_map_name_from_d_command("d.shade", shade="slope", color="aspect")
        assert result == "slope"

    def test_d_his_uses_hue_parameter(self):
        """d.his is special case using 'hue' parameter."""
        result = get_map_name_from_d_command("d.his", hue="aspect", intensity="slope")
        assert result == "aspect"

    def test_d_rgb_uses_red_parameter(self):
        """d.rgb is special case using 'red' parameter (returns first of RGB)."""
        result = get_map_name_from_d_command(
            "d.rgb", red="lsat7_2002_30", green="lsat7_2002_20", blue="lsat7_2002_10"
        )
        assert result == "lsat7_2002_30"

    def test_missing_parameter_returns_empty(self):
        """When expected parameter is missing, return empty string."""
        result = get_map_name_from_d_command("d.rast")
        assert result == ""

    def test_unknown_module_uses_map_parameter(self):
        """Unknown modules default to 'map' parameter."""
        result = get_map_name_from_d_command("d.unknown", map="test_map")
        assert result == "test_map"


class TestHtmlFormatting:
    """Tests for HTML formatting helper functions."""

    def test_style_table_adds_css(self):
        """_style_table should wrap content with CSS styles."""
        html = "<table><tr><td>test</td></tr></table>"
        result = _style_table(html)
        assert "<style>" in result
        assert "border-collapse" in result
        assert html in result

    def test_format_nested_table_creates_rows(self):
        """_format_nested_table should create table rows for attributes."""
        attributes = {"name": "road1", "type": "highway", "length": "100"}
        result = _format_nested_table(attributes)
        assert "<tr>" in result
        assert "name" in result
        assert "road1" in result
        assert "highway" in result

    def test_format_nested_table_skips_empty_values(self):
        """_format_nested_table should skip empty/None values."""
        attributes = {"name": "road1", "empty": "", "none_val": None}
        result = _format_nested_table(attributes)
        assert "name" in result
        assert "road1" in result
        # Empty values should not appear
        assert "empty" not in result

    def test_format_regular_output_extracts_category_layer(self):
        """_format_regular_output should only include Category and Layer."""
        items = [
            ("Category", "1"),
            ("Layer", "1"),
            ("OtherField", "ignored"),
        ]
        result = _format_regular_output(items)
        assert "Category" in result
        assert "Layer" in result
        assert "OtherField" not in result

    def test_format_regular_output_empty_when_no_match(self):
        """_format_regular_output returns empty when no Category/Layer."""
        items = [("name", "test"), ("value", "123")]
        result = _format_regular_output(items)
        assert result == ""
