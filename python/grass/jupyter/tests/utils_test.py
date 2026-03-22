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
    region = {"n": 4, "s": 0, "e": 8, "w": 0}

    # both provided
    assert get_rendering_size({}, 800, 600) == (800, 600)

    # width only
    width, height = get_rendering_size(region, 800, None)
    assert width == 800
    assert height == 400

    # height only
    width, height = get_rendering_size(region, None, 400)
    assert width == 800
    assert height == 400


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
