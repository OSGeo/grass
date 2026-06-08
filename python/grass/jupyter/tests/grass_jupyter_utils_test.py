"""Tests of pure helper functions in grass.jupyter.utils"""

import pytest

from grass.jupyter.utils import get_map_name_from_d_command, get_rendering_size


@pytest.mark.parametrize(
    ("region", "width", "height", "expected"),
    [
        # Both dimensions given: returned unchanged, the region is ignored.
        ({"n": 10, "s": 0, "e": 10, "w": 0}, 800, 600, (800, 600)),
        # Only width given: height follows the region aspect ratio.
        ({"n": 200, "s": 0, "e": 400, "w": 0}, 800, None, (800, 400)),
        # Only height given: width follows the region aspect ratio.
        ({"n": 200, "s": 0, "e": 400, "w": 0}, None, 300, (600, 300)),
        # Neither given, taller-than-wide region: the default height is used.
        ({"n": 400, "s": 0, "e": 200, "w": 0}, None, None, (200, 400)),
        # Neither given, wider-than-tall region: the default width is used.
        ({"n": 200, "s": 0, "e": 400, "w": 0}, None, None, (600, 300)),
        # round() is half-to-even (banker's rounding): 5 * 1 / 2 = 2.5 -> 2.
        ({"n": 1, "s": 0, "e": 2, "w": 0}, 5, None, (5, 2)),
        # ...and 7 * 1 / 2 = 3.5 -> 4.
        ({"n": 1, "s": 0, "e": 2, "w": 0}, 7, None, (7, 4)),
    ],
)
def test_get_rendering_size(region, width, height, expected):
    assert get_rendering_size(region, width, height) == expected


def test_get_rendering_size_custom_defaults():
    """Custom defaults: default_width sizes a wide region, default_height a tall one.

    Only one default applies per call; the other dimension is derived from the
    region aspect ratio.
    """
    wide = {"n": 100, "s": 0, "e": 400, "w": 0}
    tall = {"n": 400, "s": 0, "e": 100, "w": 0}
    assert get_rendering_size(
        wide, None, None, default_width=500, default_height=400
    ) == (500, 125)
    assert get_rendering_size(
        tall, None, None, default_width=500, default_height=400
    ) == (100, 400)


@pytest.mark.parametrize(
    ("module", "kwargs", "expected"),
    [
        ("d.rast", {"map": "elevation"}, "elevation"),
        ("d.vect", {"map": "roads"}, "roads"),
        # Commands whose primary raster is not named "map".
        ("d.his", {"hue": "hue_map"}, "hue_map"),
        ("d.legend", {"raster": "elevation"}, "elevation"),
        ("d.rgb", {"red": "red_map"}, "red_map"),
        # d.rgb takes several maps; only the primary (red) is returned.
        (
            "d.rgb",
            {"red": "red_map", "blue": "blue_map", "green": "green_map"},
            "red_map",
        ),
        ("d.shade", {"shade": "relief"}, "relief"),
        # Expected parameter absent: empty string, not an error.
        ("d.rast", {}, ""),
    ],
)
def test_get_map_name_from_d_command(module, kwargs, expected):
    assert get_map_name_from_d_command(module, **kwargs) == expected


@pytest.mark.parametrize("value", [123, None])
def test_get_map_name_from_d_command_non_string_returns_none(value):
    """A non-string value (including None) yields None rather than the raw object."""
    assert get_map_name_from_d_command("d.rast", map=value) is None
