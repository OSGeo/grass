"""Tests of r.mask.status"""

import pytest

try:
    import yaml
except ImportError:
    yaml = None

import grass.script as gs


def test_json_no_mask(session_no_data):
    """Check JSON format for no mask"""
    session = session_no_data
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert "present" in data
    assert "full_name" in data
    assert "is_reclass_of" in data
    assert data["present"] is False
    assert not data["full_name"]
    assert not data["is_reclass_of"]


def test_json_with_r_mask(session_with_data):
    """Check JSON format for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is True
    assert data["full_name"] == "MASK@PERMANENT"
    assert data["is_reclass_of"] == "a@PERMANENT"
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is False
    assert not data["full_name"]
    assert not data["is_reclass_of"]


def test_json_with_g_copy(session_with_data):
    """Check JSON format for the low-level g.copy case"""
    session = session_with_data
    gs.run_command("g.copy", raster="a,MASK", env=session.env)
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is True
    assert data["full_name"] == "MASK@PERMANENT"
    assert not data["is_reclass_of"]
    # Now remove the mask.
    gs.run_command("g.remove", type="raster", name="MASK", flags="f", env=session.env)
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is False
    assert not data["full_name"]
    assert not data["is_reclass_of"]


def test_shell_with_r_mask(session_with_data):
    """Check shell format for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    data = gs.parse_command("r.mask.status", format="bash", env=session.env)
    assert int(data["present"])
    assert data["full_name"] == "MASK@PERMANENT"
    assert data["is_reclass_of"] == "a@PERMANENT"
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    data = gs.parse_command("r.mask.status", format="bash", env=session.env)
    assert not int(data["present"])
    assert not data["full_name"]
    assert not data["is_reclass_of"]


@pytest.mark.skipif(yaml is None, reason="PyYAML package not available")
def test_yaml_with_r_mask(session_with_data):
    """Check YAML format for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    text = gs.read_command("r.mask.status", format="yaml", env=session.env)
    data = yaml.safe_load(text)
    assert data["present"] is True
    assert data["full_name"] == "MASK@PERMANENT"
    assert data["is_reclass_of"] == "a@PERMANENT"
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    text = gs.read_command("r.mask.status", format="yaml", env=session.env)
    data = yaml.safe_load(text)
    assert data["present"] is False
    assert not data["full_name"]
    assert not data["is_reclass_of"]
