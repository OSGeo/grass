"""Tests for grass.jupyter.utils."""

import pytest
import grass.script as gs
from grass.jupyter.utils import (
    get_region,
    get_rendering_size,
    get_map_name_from_d_command,
)


def test_get_region(session_with_data):
    """get_region() should match GRASS region output."""
    env = session_with_data.env

    expected = gs.region(env=env)
    result = get_region(env=env)

    assert result["north"] == pytest.approx(expected["n"])
    assert result["south"] == pytest.approx(expected["s"])
    assert result["east"] == pytest.approx(expected["e"])
    assert result["west"] == pytest.approx(expected["w"])


def test_get_rendering_size():
    """Test rendering size calculations."""
    wide_region = {"n": 4, "s": 0, "e": 8, "w": 0}
    tall_region = {"n": 8, "s": 0, "e": 4, "w": 0}

    # both provided
    assert get_rendering_size({}, 800, 600) == (800, 600)

    # width only
    assert get_rendering_size(wide_region, 800, None) == (800, 400)

    # height only
    assert get_rendering_size(wide_region, None, 400) == (800, 400)

    # neither provided - wide region
    assert get_rendering_size(wide_region, None, None) == (600, 300)

    # neither provided - tall region
    assert get_rendering_size(tall_region, None, None) == (200, 400)


@pytest.mark.parametrize(
    ("cmd", "kwargs", "expected"),
    [
        ("d.rast", {"map": "elevation"}, "elevation"),
        ("d.rgb", {"red": "red_band"}, "red_band"),
        ("d.legend", {"raster": "elevation"}, "elevation"),
    ],
)
def test_get_map_name_from_d_command(cmd, kwargs, expected):
    """Test map name extraction from display commands."""
    assert get_map_name_from_d_command(cmd, **kwargs) == expected
