import os

import pytest

from grass.app.runtime import RuntimePaths


def test_attr_access_does_not_modify_env():
    """Accessing attribute should not change environment."""
    env = {}
    rp = RuntimePaths(env=env)
    assert "GRASS_COLORSDIR" not in env
    value = rp.colors_dir  # access the attribute
    assert value
    assert "GRASS_COLORSDIR" not in env, "env was modified unexpectedly"


def test_explicit_env_vars_set():
    """Explicit call should set the env vars."""
    env = {}
    paths = RuntimePaths(env=env)
    paths.set_env_vars()
    assert "GRASS_COLORSDIR" in env
    assert env["GRASS_COLORSDIR"] == paths.colors_dir


def test_constructor_parameter_env_vars_set():
    """Constructor with parameter should set the env vars."""
    env = {}
    paths = RuntimePaths(env=env, init_env_vars=True)
    assert "GRASS_COLORSDIR" in env
    assert env["GRASS_COLORSDIR"] == paths.colors_dir


def test_dir_lists_dynamic_attributes_but_does_not_modify_env():
    """dir() should show dynamic attrs but not set env."""
    env = {}
    paths = RuntimePaths(env=env)
    listing = dir(paths)
    assert "colors_dir" in listing
    assert "GRASS_COLORSDIR" not in env, "dir() should not modify env"


def test_existing_env_value_is_respected():
    """If env already contains GRASS_COLORSDIR, its value is used."""
    value = "/custom/colors"
    env = {"GRASS_COLORSDIR": value}
    paths = RuntimePaths(env=env)
    assert paths.colors_dir == os.path.normpath(value)
    assert env["GRASS_COLORSDIR"] == value


def test_invalid_attribute_raises():
    """Unknown attribute access should raise AttributeError."""
    rp = RuntimePaths(env={})
    with pytest.raises(AttributeError):
        assert rp.unknown_attr  # avoiding unused value


def test_returned_attribute_consistent():
    """Repeated accesses should return the same value."""
    paths = RuntimePaths(env={})
    first = paths.colors_dir
    second = paths.colors_dir
    assert first == second
    assert first == RuntimePaths(env={}).colors_dir
