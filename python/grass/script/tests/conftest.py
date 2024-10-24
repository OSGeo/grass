"""Fixtures for grass.script"""

import os

import pytest

import grass.script as gs


@pytest.fixture
def mock_no_session(monkeypatch):
    """Set the environment variables as if there would be no background session.

    Use with usefixtures (not as a paramter) to avoid warnings about an unused
    parameter::

        @pytest.mark.usefixtures("mock_no_session")
        def test_session_handling():
            pass

    There may or may not be a session in the background (we don't check either way).
    """
    monkeypatch.delenv("GISRC", raising=False)
    monkeypatch.delenv("GISBASE", raising=False)


@pytest.fixture
def session(tmp_path):
    """Set up a GRASS session for the tests."""
    project = tmp_path / "test_project"

    # Create a test location
    gs.create_project(project)

    # Initialize the GRASS session
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session
