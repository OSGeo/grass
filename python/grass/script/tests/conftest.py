"""Fixtures for grass.script"""

import pytest


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
