"""Tests of runtime setup, mostly focused on paths

The install path (aka GISBASE) tests are assuming, or focusing on a non-FHS
installation. Their potential to fail is with broken builds and installs.
"""

import os
from pathlib import Path

import pytest

from grass.app import resource_paths
from grass.app.runtime import (
    RuntimePaths,
    set_dynamic_library_path,
    set_executable_paths,
    set_python_path_variable,
)
from grass.script.setup import get_install_path


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


def test_executable_paths_added_once(tmp_path):
    """Repeated calls do not add the executable paths again

    A long-lived process such as a pytest run or a Jupyter kernel calls the
    setup repeatedly on the same environment, and the variable used to grow
    with every call.
    """
    env = {"PATH": "/usr/bin"}
    set_executable_paths(str(tmp_path), str(tmp_path / "config"), env=env)
    after_first = env["PATH"]
    assert after_first != "/usr/bin"
    assert after_first.endswith("/usr/bin")

    set_executable_paths(str(tmp_path), str(tmp_path / "config"), env=env)
    assert env["PATH"] == after_first


def test_executable_paths_of_another_installation_are_added_in_front(tmp_path):
    """Switching to another installation still puts its paths first"""
    env = {"PATH": "/usr/bin"}
    first = tmp_path / "first"
    second = tmp_path / "second"
    set_executable_paths(str(first), str(tmp_path / "config"), env=env)
    set_executable_paths(str(second), str(tmp_path / "config"), env=env)

    entries = env["PATH"].split(os.pathsep)
    assert str(second / "bin") in entries
    assert entries.index(str(second / "bin")) < entries.index(str(first / "bin"))


def test_dynamic_library_path_added_once(tmp_path):
    """Repeated calls do not add the library path again"""
    env = {}
    set_dynamic_library_path("LD_LIBRARY_PATH", str(tmp_path), env=env)
    after_first = env["LD_LIBRARY_PATH"]

    set_dynamic_library_path("LD_LIBRARY_PATH", str(tmp_path), env=env)
    assert env["LD_LIBRARY_PATH"] == after_first


def test_dynamic_library_path_has_no_empty_entry(tmp_path):
    """An unset variable does not gain a leading empty entry

    An empty entry in a library path is the current directory, which is not
    what the setup means to add.
    """
    env = {}
    set_dynamic_library_path("LD_LIBRARY_PATH", str(tmp_path), env=env)
    assert env["LD_LIBRARY_PATH"] == os.path.join(str(tmp_path), "lib")
    assert "" not in env["LD_LIBRARY_PATH"].split(os.pathsep)


def test_dynamic_library_path_keeps_existing_value(tmp_path):
    """An existing value is kept and the library path is added to it"""
    env = {"LD_LIBRARY_PATH": "/existing/lib"}
    set_dynamic_library_path("LD_LIBRARY_PATH", str(tmp_path), env=env)
    assert env["LD_LIBRARY_PATH"].split(os.pathsep) == [
        "/existing/lib",
        os.path.join(str(tmp_path), "lib"),
    ]


def test_python_path_variable_added_once(tmp_path):
    """Repeated calls do not add the Python path again"""
    env = {}
    set_python_path_variable(str(tmp_path), env=env)
    after_first = env["PYTHONPATH"]
    assert after_first == os.path.join(str(tmp_path), "etc", "python")

    set_python_path_variable(str(tmp_path), env=env)
    assert env["PYTHONPATH"] == after_first


def test_python_path_variable_keeps_existing_value(tmp_path):
    """An existing value is kept and the Python path is added in front"""
    env = {"PYTHONPATH": "/existing/python"}
    set_python_path_variable(str(tmp_path), env=env)
    assert env["PYTHONPATH"].split(os.pathsep) == [
        os.path.join(str(tmp_path), "etc", "python"),
        "/existing/python",
    ]
