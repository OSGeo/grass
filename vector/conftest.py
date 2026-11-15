"""Fixtures for v.edit tool tests"""

import pytest
import os

import grass.script as gs


@pytest.fixture
def xy_dataset_session(tmp_path):
    """Creates a session with XY project"""
    gs.create_project(tmp_path / "test")
    with gs.setup.init(tmp_path / "test", env=os.environ.copy()) as session:
        yield session
