"""Test grass.script.core.get_commands function"""

import sys

import pytest

import grass.script as gs


def common_test_code(executables_set, scripts_dict):
    """Assert results

    Assuming we should always have some executables on path for this to pass a test.
    """
    assert len(executables_set)
    assert "g.region" in executables_set
    if sys.platform == "win32":
        assert len(scripts_dict.items())
        # Just in case this is not a standard dictionary object.
        assert len(scripts_dict.keys()) == len(scripts_dict.items())
        assert "r.shade" in scripts_dict["py"]
    else:
        assert "r.shade" in executables_set


def test_no_explicit_session():
    """Test without explicit session setup here"""
    executables_set, scripts_dict = gs.get_commands()
    common_test_code(executables_set, scripts_dict)


@pytest.mark.usefixtures("mock_no_session")
def test_no_session_mocked():
    """Test with mocked no session"""
    executables_set, scripts_dict = gs.get_commands()
    common_test_code(executables_set, scripts_dict)


@pytest.mark.usefixtures("mock_no_session")
def test_in_session(tmp_path):
    """Test with mocked no session"""
    # TODO: Use env=os.environ.copy()
    project = tmp_path / "project"
    gs.create_project(project)
    with gs.setup.init(project) as session:
        executables_set, scripts_dict = gs.get_commands(env=session.env)
    common_test_code(executables_set, scripts_dict)
