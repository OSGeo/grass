"""Tests of runtime setup, mostly focused on paths

The install path (aka GISBASE) tests are assuming, or focusing on a non-FHS
installation. Their potential to fail is with broken builds and installs.
"""

from pathlib import Path

import pytest

from grass.app.runtime import RuntimePaths
from grass.script.setup import get_install_path


def return_as_is(x):
    """Return the parameter exactly as recieved"""
    return x


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


def test_consitent_install_path_returned():
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
