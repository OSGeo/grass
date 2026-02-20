"""Test Utils funcitons"""

import pytest

import grass.script as gs

from grass.jupyter.utils import get_region, get_location_proj_string

IPython = pytest.importorskip("IPython", reason="IPython package not available")
ipywidgets = pytest.importorskip(
    "ipywidgets", reason="ipywidgets package not available"
)


def test_get_region(tmp_path):
    """Test that get_region returns currnt computational region as dictionary."""
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project) as session:
        region = get_region(session.env)
        assert isinstance(region, dict)
        assert "north" in region
        assert "south" in region
        assert "east" in region
        assert "west" in region


def test_get_location_proj_string(tmp_path):
    """Test that get_location_proj_string returns projection of environment in PROJ.4 format"""
    project = tmp_path / "test_project_proj"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.proj", flags="c", epsg="4326")
        projection = get_location_proj_string()
        assert "+proj=" in projection
