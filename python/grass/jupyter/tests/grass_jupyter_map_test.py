from pathlib import Path

import pytest

import grass.jupyter as gj


@pytest.mark.parametrize("use_region", [True, False])
def test_created_with_env(session_with_data, use_region):
    """Check that object can be used with environment"""
    map2d = gj.Map(env=session_with_data.env, use_region=use_region)
    map2d.d_rast(map="data")

    image = Path(map2d.filename)
    assert image.exists()
    assert image.stat().st_size


@pytest.mark.parametrize("use_region", [True, False])
def test_created_with_session(session_with_data, use_region):
    """Check that object can be used with a session object"""
    map2d = gj.Map(session=session_with_data, use_region=use_region)
    map2d.d_rast(map="data")

    image = Path(map2d.filename)
    assert image.exists()
    assert image.stat().st_size


def test_attribute_access_c_tools():
    """Check C tool names can be listed without a session"""
    map2d = gj.Map()
    assert "d_rast" in dir(map2d)
    assert "d_vect" in dir(map2d)


def test_attribute_access_python_tools():
    """Check Python tool names can be listed without a session"""
    map2d = gj.Map()
    assert "d_background" in dir(map2d)
    assert "d_shade" in dir(map2d)


def test_non_display_tools():
    """Check non-display tools are not listed"""
    map2d = gj.Map()
    assert "r_info" not in dir(map2d)
    assert "db_univar" not in dir(map2d)
