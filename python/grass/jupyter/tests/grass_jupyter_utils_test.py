"""Test Utils funcitons"""

import pytest

from grass.jupyter.utils import get_region, get_location_proj_string

IPython = pytest.importorskip("IPython", reason="IPython package not available")
ipywidgets = pytest.importorskip(
"ipywidgets", reason="ipywidgets package not available"
)

def test_get_region(session):
    """Test that get_region returns currnt computational region as dictionary."""
    region = get_region(session.env)
    assert isinstance(region, dict)
    assert "north" in region
    assert "south" in region
    assert "east" in region
    assert "west" in region

def test_get_location_proj_string(simple_dataset):
    """Test that get_location_proj_string returns projection of environment in PROJ.4 format"""
    projection = get_location_proj_string()
    assert "+proj=" in projection

