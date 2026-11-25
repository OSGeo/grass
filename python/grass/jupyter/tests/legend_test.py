"""Tests for grass.jupyter.legend module

Tests cover:
- Categorical HTML generation (color boxes)
- Continuous HTML generation (gradient bar with interpolated colors)
- generate_legend_html routing logic
- parse_colors with a real GRASS raster (requires session)
"""

import pytest

from grass.jupyter.legend import (
    _generate_categorical_html,
    _generate_continuous_html,
    generate_legend_html,
    parse_colors,
)


# ---------- Unit tests (no GRASS session needed) ----------


class TestCategoricalHtml:
    """Tests for _generate_categorical_html."""

    @pytest.fixture
    def sample_items(self):
        return [
            {"value": 1, "label": "Class 1", "rgb": (255, 0, 0)},
            {"value": 2, "label": "Class 2", "rgb": (0, 255, 0)},
            {"value": 3, "label": "Class 3", "rgb": (0, 0, 255)},
        ]

    def test_contains_title(self, sample_items):
        html = _generate_categorical_html(sample_items, "Land Cover")
        assert "<strong>Land Cover</strong>" in html

    def test_contains_all_labels(self, sample_items):
        html = _generate_categorical_html(sample_items, "Test")
        assert "Class 1" in html
        assert "Class 2" in html
        assert "Class 3" in html

    def test_contains_rgb_colors(self, sample_items):
        html = _generate_categorical_html(sample_items, "Test")
        assert "rgb(255,0,0)" in html
        assert "rgb(0,255,0)" in html
        assert "rgb(0,0,255)" in html

    def test_has_maplegend_class(self, sample_items):
        html = _generate_categorical_html(sample_items, "Test")
        assert 'class="maplegend leaflet-control"' in html

    def test_has_pointer_events(self, sample_items):
        html = _generate_categorical_html(sample_items, "Test")
        assert "pointer-events: auto" in html

    def test_empty_items(self):
        html = _generate_categorical_html([], "Empty")
        assert "<strong>Empty</strong>" in html


class TestContinuousHtml:
    """Tests for _generate_continuous_html."""

    @pytest.fixture
    def sample_items(self):
        return [
            {"value": 0.0, "label": "0", "rgb": (0, 0, 255)},
            {"value": 50.0, "label": "50", "rgb": (0, 255, 0)},
            {"value": 100.0, "label": "100", "rgb": (255, 0, 0)},
        ]

    def test_contains_title(self, sample_items):
        html = _generate_continuous_html(sample_items, "Elevation")
        assert "<strong>Elevation</strong>" in html

    def test_empty_items_returns_empty(self):
        assert _generate_continuous_html([], "Test") == ""

    def test_has_maplegend_class(self, sample_items):
        html = _generate_continuous_html(sample_items, "Test")
        assert 'class="maplegend leaflet-control"' in html

    def test_contains_tick_labels(self, sample_items):
        html = _generate_continuous_html(sample_items, "Test")
        # Should contain min and max labels at minimum
        assert "0" in html
        assert "100" in html

    def test_contains_interpolated_color_blocks(self, sample_items):
        html = _generate_continuous_html(sample_items, "Test")
        # Should contain multiple rgb() background colors from interpolation
        assert "background: rgb(" in html
        # Should contain the exact endpoint colors
        assert "rgb(255,0,0)" in html
        assert "rgb(0,0,255)" in html

    def test_gradient_has_flex_container(self, sample_items):
        html = _generate_continuous_html(sample_items, "Test")
        assert "flex-direction: column" in html

    def test_interpolation_produces_intermediate_colors(self, sample_items):
        """Verify interpolation produces colors between endpoints."""
        html = _generate_continuous_html(sample_items, "Test")
        # With 50 steps, there should be many distinct rgb() values
        # beyond just the two endpoints
        rgb_count = html.count("background: rgb(")
        assert rgb_count == 50

    def test_single_item(self):
        items = [{"value": 42.0, "label": "42", "rgb": (128, 128, 128)}]
        html = _generate_continuous_html(items, "Single")
        assert "rgb(128,128,128)" in html

    def test_unsorted_items_are_sorted(self):
        """Items should be sorted by value regardless of input order."""
        items = [
            {"value": 100.0, "label": "100", "rgb": (255, 0, 0)},
            {"value": 0.0, "label": "0", "rgb": (0, 0, 255)},
            {"value": 50.0, "label": "50", "rgb": (0, 255, 0)},
        ]
        html = _generate_continuous_html(items, "Unsorted")
        # Endpoints must be present regardless of input order
        assert "rgb(255,0,0)" in html
        assert "rgb(0,0,255)" in html
        # Should have 50 interpolated color blocks
        assert html.count("background: rgb(") == 50


class TestGenerateLegendHtml:
    """Tests for the generate_legend_html routing function."""

    def test_routes_to_categorical(self):
        parsed = {
            "type": "categorical",
            "items": [
                {"value": 1, "label": "Class 1", "rgb": (255, 0, 0)},
                {"value": 2, "label": "Class 2", "rgb": (0, 255, 0)},
            ],
            "nv": None,
            "default": None,
        }
        html = generate_legend_html(parsed, title="Categories")
        # Categorical uses color boxes with 14px spans
        assert "width: 14px" in html
        assert "Class 1" in html

    def test_routes_to_continuous(self):
        parsed = {
            "type": "continuous",
            "items": [
                {"value": 0.0, "label": "0", "rgb": (0, 0, 255)},
                {"value": 100.0, "label": "100", "rgb": (255, 0, 0)},
            ],
            "nv": None,
            "default": None,
        }
        html = generate_legend_html(parsed, title="Gradient")
        # Continuous uses flex column gradient bar
        assert "flex-direction: column" in html

    def test_max_items_limits_categorical(self):
        items = [
            {"value": i, "label": f"Class {i}", "rgb": (i * 10, 0, 0)}
            for i in range(20)
        ]
        parsed = {
            "type": "categorical",
            "items": items,
            "nv": None,
            "default": None,
        }
        html = generate_legend_html(parsed, title="Many", max_items=5)
        # Should not contain all 20 items
        assert html.count("Class ") < 20

    def test_custom_title(self):
        parsed = {
            "type": "categorical",
            "items": [{"value": 1, "label": "A", "rgb": (0, 0, 0)}],
            "nv": None,
            "default": None,
        }
        html = generate_legend_html(parsed, title="My Custom Title")
        assert "My Custom Title" in html


# ---------- Integration tests (require GRASS session) ----------


class TestParseColors:
    """Tests for parse_colors with real GRASS rasters."""

    def test_parse_returns_dict(self, simple_dataset):
        """parse_colors should return a dict with expected keys."""
        result = parse_colors(simple_dataset.raster_name)
        assert isinstance(result, dict)
        assert "type" in result
        assert "items" in result
        assert "nv" in result
        assert "default" in result

    def test_parse_has_items(self, simple_dataset):
        """Parsed result should have at least one color item."""
        result = parse_colors(simple_dataset.raster_name)
        assert len(result["items"]) > 0

    def test_items_have_required_keys(self, simple_dataset):
        """Each item should have value, label, and rgb keys."""
        result = parse_colors(simple_dataset.raster_name)
        for item in result["items"]:
            assert "value" in item
            assert "label" in item
            assert "rgb" in item

    def test_rgb_is_tuple_of_three(self, simple_dataset):
        """RGB values should be 3-element tuples."""
        result = parse_colors(simple_dataset.raster_name)
        for item in result["items"]:
            assert isinstance(item["rgb"], tuple)
            assert len(item["rgb"]) == 3

    def test_type_is_valid(self, simple_dataset):
        """Type should be either categorical or continuous."""
        result = parse_colors(simple_dataset.raster_name)
        assert result["type"] in {"categorical", "continuous"}

    def test_end_to_end_html(self, simple_dataset):
        """Full pipeline: parse_colors -> generate_legend_html."""
        parsed = parse_colors(simple_dataset.raster_name)
        html = generate_legend_html(parsed, title="Test Legend")
        assert "<strong>Test Legend</strong>" in html
        assert 'class="maplegend' in html
