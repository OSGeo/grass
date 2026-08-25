"""Tests of r.mask.status"""

import pytest

try:
    import yaml
except ImportError:
    yaml = None

import grass.script as gs


DEFAULT_MASK_NAME = "MASK"


def test_json_no_mask(session_no_data):
    """Check JSON format for no mask"""
    session = session_no_data
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert "present" in data
    assert "name" in data
    assert data["name"], "Mask name needs to be always set"
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert "is_reclass_of" in data
    assert data["present"] is False
    assert not data["is_reclass_of"]


def test_json_with_r_mask(session_with_data):
    """Check JSON format for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is True
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert data["is_reclass_of"] == "a@PERMANENT"
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is False
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert not data["is_reclass_of"]


def test_json_with_g_copy(session_with_data):
    """Check JSON format for the low-level g.copy case"""
    session = session_with_data
    gs.run_command("g.copy", raster=["a", DEFAULT_MASK_NAME], env=session.env)
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is True
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert not data["is_reclass_of"]
    # Now remove the mask.
    gs.run_command(
        "g.remove", type="raster", name=DEFAULT_MASK_NAME, flags="f", env=session.env
    )
    data = gs.parse_command("r.mask.status", format="json", env=session.env)
    assert data["present"] is False
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert not data["is_reclass_of"]


def test_shell(session_with_data):
    """Check shell format for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    data = gs.parse_command("r.mask.status", format="shell", env=session.env)
    assert int(data["present"])
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert data["is_reclass_of"] == "a@PERMANENT"
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    data = gs.parse_command("r.mask.status", format="shell", env=session.env)
    assert not int(data["present"])
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert not data["is_reclass_of"]


@pytest.mark.skipif(yaml is None, reason="PyYAML package not available")
def test_yaml(session_with_data):
    """Check YAML format for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    text = gs.read_command("r.mask.status", format="yaml", env=session.env)
    data = yaml.safe_load(text)
    assert data["present"] is True
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert data["is_reclass_of"] == "a@PERMANENT"
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    text = gs.read_command("r.mask.status", format="yaml", env=session.env)
    data = yaml.safe_load(text)
    assert data["present"] is False
    assert data["name"] == f"{DEFAULT_MASK_NAME}@PERMANENT"
    assert not data["is_reclass_of"]


def test_plain(session_with_data):
    """Check plain text format for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    text = gs.read_command("r.mask.status", format="plain", env=session.env)
    assert text
    assert f"{DEFAULT_MASK_NAME}@PERMANENT" in text
    assert "a@PERMANENT" in text
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    text = gs.read_command("r.mask.status", format="plain", env=session.env)
    assert text
    assert f"{DEFAULT_MASK_NAME}@PERMANENT" in text
    assert "a@PERMANENT" not in text


def test_without_parameters(session_no_data):
    """Check output is generated with no parameters"""
    session = session_no_data
    text = gs.read_command("r.mask.status", env=session.env)
    assert text


def test_behavior_mimicking_test_program(session_with_data):
    """Check test program like behavior for the r.mask case"""
    session = session_with_data
    gs.run_command("r.mask", raster="a", env=session.env)
    returncode = gs.run_command(
        "r.mask.status", flags="t", env=session.env, errors="status"
    )
    assert returncode == 0
    # Now remove the mask.
    gs.run_command("r.mask", flags="r", env=session.env)
    returncode = gs.run_command(
        "r.mask.status", flags="t", env=session.env, errors="status"
    )
    assert returncode == 1
