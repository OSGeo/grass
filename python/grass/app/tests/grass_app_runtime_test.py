"""Tests of runtime setup, mostly focused on paths

The install path (aka GISBASE) tests are assuming, or focusing on a non-FHS
installation. Their potential to fail is with broken builds and installs.
"""

import os
from pathlib import Path

import pytest

from grass.app import resource_paths
from grass.app.runtime import RuntimePaths, set_proj_data_for_conda
from grass.script.setup import get_install_path, setup_runtime_env


def return_as_is(x):
    """Return the parameter exactly as received"""
    return x


def test_prefix_set():
    """Check that the prefix attribute is set by default"""
    paths = RuntimePaths(env={})
    assert paths.prefix


@pytest.mark.parametrize("path_type", [str, Path])
@pytest.mark.parametrize(
    "custom_prefix",
    ["/custom/prefix/path", "/custom/prefix/path/", "/path with spaces"],
)
def test_custom_prefix_set(custom_prefix, path_type):
    """Check that the prefix attribute is set from constructor"""
    paths = RuntimePaths(env={}, prefix=path_type(custom_prefix))
    assert paths.prefix == os.path.normpath(custom_prefix)


def test_gisbase_prefixed():
    """Check that GISBASE should start with prefix"""
    paths = RuntimePaths(env={})
    assert paths.gisbase.startswith(paths.prefix)


@pytest.mark.parametrize(
    "custom_prefix",
    ["/custom/prefix/path", "/custom/prefix/path/", "/path with spaces"],
)
def test_gisbase_with_custom_prefix(custom_prefix):
    """Check that GISBASE should start with custom prefix"""
    paths = RuntimePaths(env={}, prefix=custom_prefix)
    assert paths.gisbase.startswith(os.path.normpath(custom_prefix))


@pytest.mark.parametrize(
    "custom_prefix",
    ["/custom/prefix/path", "/custom/prefix/path/", "/path with spaces"],
)
def test_gisbase_and_prefix_mix(custom_prefix):
    """Check passing a custom prefix which is actually GISBASE"""
    # resource_paths.GISBASE is just the unique part after the prefix.
    custom_prefix = os.path.join(custom_prefix, resource_paths.GISBASE)
    paths = RuntimePaths(env={}, prefix=custom_prefix)
    assert paths.gisbase == os.path.normpath(custom_prefix)


@pytest.mark.parametrize("path_type", [str, Path])
@pytest.mark.parametrize(
    "custom_prefix",
    ["/custom/prefix/path", "/custom/prefix/path/", "/path with spaces"],
)
def test_env_gisbase_with_custom_prefix(custom_prefix, path_type):
    """Check that GISBASE should start with custom prefix"""
    env = {}
    RuntimePaths(env=env, prefix=path_type(custom_prefix), set_env_variables=True)
    assert env["GISBASE"].startswith(os.path.normpath(custom_prefix))


def test_attr_access_does_not_modify_env():
    """Accessing attribute should not change environment."""
    env = {}
    paths = RuntimePaths(env=env)
    assert "GISBASE" not in env
    value = paths.gisbase  # access the attribute
    assert value
    assert "GISBASE" not in env, "env was modified unexpectedly"


def test_explicit_env_vars_set():
    """Explicit call should set the env vars."""
    env = {}
    paths = RuntimePaths(env=env)
    paths.set_env_variables()
    assert "GISBASE" in env
    assert env["GISBASE"] == paths.gisbase


def test_constructor_parameter_env_vars_set():
    """Constructor with parameter should set the env vars."""
    env = {}
    paths = RuntimePaths(env=env, set_env_variables=True)
    assert "GISBASE" in env
    assert env["GISBASE"] == paths.gisbase


def test_dir_lists_dynamic_attributes_but_does_not_modify_env():
    """dir() should show dynamic attrs but not set env."""
    env = {}
    paths = RuntimePaths(env=env)
    listing = dir(paths)
    assert "gisbase" in listing
    assert "GISBASE" not in env, "dir() should not modify env"


def test_existing_env_value_is_respected():
    """If env already contains GISBASE, its value is used."""
    value = "/custom/path/to/grass"
    env = {"GISBASE": value}
    paths = RuntimePaths(env=env)
    assert paths.gisbase == os.path.normpath(value)
    assert env["GISBASE"] == value


def test_invalid_attribute_raises():
    """Unknown attribute access should raise AttributeError."""
    paths = RuntimePaths(env={})
    with pytest.raises(AttributeError):
        assert paths.unknown_attribute  # avoiding unused value


def test_returned_attribute_consistent():
    """Repeated accesses should return the same value."""
    paths = RuntimePaths(env={})
    first = paths.gisbase
    second = paths.gisbase
    assert first == second
    assert first == RuntimePaths(env={}).gisbase


def test_install_path_consistent():
    """Differently sourced install paths should be the same.

    Dynamically determined install path and compile-time install path should be
    the same in a healthy installation.

    This is a non-FHS oriented test.
    """
    assert get_install_path() == RuntimePaths().gisbase


@pytest.mark.parametrize("path_type", [str, Path, return_as_is])
def test_install_path_used_as_result(path_type):
    """Passing a valid compile-time install path should return the same path.

    If the path is not recognized as install path or there is a problem with the
    dynamic determination of the install path, the test will fail.

    This is a non-FHS oriented test.
    """
    path = RuntimePaths().gisbase
    assert get_install_path(path_type(path)) == path


def test_consistent_install_path_returned():
    """Two subsequent calls should return the same result.

    The environment should not be modified by the call, so the result should
    always be the same.

    This is a non-FHS oriented test.
    """
    assert get_install_path() == get_install_path()


@pytest.mark.parametrize("path_type", [str, Path, return_as_is])
def test_feeding_output_as_input_again(path_type):
    """Passing result of the get_install_path back to it should give the same result.

    When a dynamically path gets returned by the function, the same path should be
    the one returned again when called with that path (sort of like calling the same
    function twice because we can't tell here if it is a newly computed path or the
    provided path if they are the same).

    We use this to test different path types.

    This is a non-FHS oriented test.
    """
    path = get_install_path()
    assert path == get_install_path(path_type(path))


@pytest.mark.parametrize("path_type", [str, Path, return_as_is])
def test_passing_non_existent_path(path_type):
    """Passing result of the get_install_path back to it should give the same result.

    When a dynamically path gets returned by the function, the same path should be
    the one returned again when called with that path (sort of like calling the same
    function twice because we can't tell here if it is a newly computed path or the
    provided path if they are the same).

    This is a non-FHS oriented test.
    """
    assert get_install_path(path_type("/does/not/exist")) == get_install_path()


# GISBASE relative to the prefix of the fake environment.
# resource_paths.GISBASE cannot be used because it is empty in Autotools builds.
FAKE_GISBASE = os.path.join("lib", "grass")


def make_fake_conda_env(path, windows=False, proj_db=True):
    """Create a fake conda environment for set_proj_data_for_conda()

    Only what the tested function looks at is created: the conda-meta
    directory, the GRASS installation directory, and the PROJ data directory
    with an empty proj.db (unless proj_db is false).

    Returns the GRASS installation path and the PROJ data directory.
    """
    (path / "conda-meta").mkdir(parents=True)
    if windows:
        install_path = path / "Library" / FAKE_GISBASE
        proj_data = path / "Library" / "share" / "proj"
    else:
        install_path = path / FAKE_GISBASE
        proj_data = path / "share" / "proj"
    install_path.mkdir(parents=True, exist_ok=True)
    proj_data.mkdir(parents=True)
    if proj_db:
        (proj_data / "proj.db").write_bytes(b"")
    return install_path, proj_data


@pytest.mark.parametrize("windows", [False, True])
def test_proj_data_set_in_conda(tmp_path, windows):
    """PROJ_DATA is set to the PROJ directory of the environment"""
    install_path, proj_data = make_fake_conda_env(tmp_path / "env", windows=windows)
    env = {}
    # GISBASE is a string in the callers.
    set_proj_data_for_conda(str(install_path), env=env)
    assert env == {"PROJ_DATA": str(proj_data.resolve())}


@pytest.mark.parametrize("variable", ["PROJ_DATA", "PROJ_LIB"])
def test_proj_data_existing_value_is_respected(tmp_path, variable):
    """A PROJ variable set by the user or the conda activation script is kept"""
    install_path, _ = make_fake_conda_env(tmp_path / "env")
    env = {variable: "/elsewhere"}
    set_proj_data_for_conda(install_path, env=env)
    assert env == {variable: "/elsewhere"}


def test_proj_data_not_set_without_proj_db(tmp_path):
    """A PROJ directory without proj.db is not used"""
    install_path, _ = make_fake_conda_env(tmp_path / "env", proj_db=False)
    env = {}
    set_proj_data_for_conda(install_path, env=env)
    assert env == {}


def test_proj_data_not_set_outside_conda(tmp_path):
    """Installations outside of conda are not affected"""
    install_path = tmp_path / FAKE_GISBASE
    install_path.mkdir(parents=True)
    proj_data = tmp_path / "share" / "proj"
    proj_data.mkdir(parents=True)
    (proj_data / "proj.db").write_bytes(b"")
    env = {}
    set_proj_data_for_conda(install_path, env=env)
    assert env == {}


def test_setup_runtime_env_sets_proj_data_in_conda(tmp_path, monkeypatch):
    """Runtime setup for a GRASS in conda sets PROJ_DATA"""
    # The runtime setup derives the prefix by removing the build-time
    # GISBASE from the end of the given path.
    monkeypatch.setattr(resource_paths, "GISBASE", FAKE_GISBASE)
    install_path, proj_data = make_fake_conda_env(tmp_path / "env")
    env = os.environ.copy()
    # GISBASE from the environment would take precedence over the path.
    for variable in ("GISBASE", "PROJ_DATA", "PROJ_LIB"):
        env.pop(variable, None)
    setup_runtime_env(install_path, env=env)
    assert env["PROJ_DATA"] == str(proj_data.resolve())
