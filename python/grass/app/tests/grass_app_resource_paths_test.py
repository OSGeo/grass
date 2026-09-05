from pathlib import Path

import pytest

from grass.app import resource_paths


# This is just a sample, not trying to test all.
@pytest.mark.parametrize(
    "name",
    [
        "GRASS_VERSION",
        "GRASS_VERSION_MAJOR",
        "LD_LIBRARY_PATH_VAR",
        "GRASS_VERSION_GIT",
    ],
)
def test_non_path_values_substituted(name):
    value = getattr(resource_paths, name)
    assert not (value.startswith("@") and value.endswith("@"))


# This is just a sample, not trying to test all.
@pytest.mark.parametrize("name", ["GRASS_PREFIX", "GISBASE"])
def test_path_values_substituted(name):
    value = getattr(resource_paths, name)
    assert not (value.startswith("@") and value.endswith("@"))


# GISBASE may be empty after build because everything goes to prefix.
@pytest.mark.parametrize("name", ["GRASS_PREFIX"])
def test_value_not_empty(name):
    value = getattr(resource_paths, name)
    assert value


@pytest.mark.parametrize("name", ["GRASS_PREFIX"])
def test_value_is_directory(name):
    value = getattr(resource_paths, name)
    assert Path(value).exists()
    assert Path(value).is_dir()
