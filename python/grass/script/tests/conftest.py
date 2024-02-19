"""Fixtures for Jupyter tests

Fixture for grass.jupyter.TimeSeries test

Fixture for ReprojectionRenderer test with simple GRASS location, raster, vector.
"""

import pytest


@pytest.fixture
def mock_no_session(monkeypatch):
    """Set the environment variables as if there would be no background session.

    There may or may not be a session in the background (we don't check either way).
    """
    monkeypatch.delenv("GISRC", raising=False)
    monkeypatch.delenv("GISBASE", raising=False)
