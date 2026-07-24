"""Test g.extension"""

import os

import pytest

import grass.script as gs


# Using module-scoped fixture to avoid overhead of downloading addons repo.
@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Session with modified addon base directory (scope: module)"""
    tmp_path = tmp_path_factory.mktemp("g_extension_tests")
    gs.create_project(tmp_path / "test")
    env = os.environ.copy()
    env["GRASS_ADDON_BASE"] = str(tmp_path / "addons")
    with gs.setup.init(tmp_path / "test", env=env) as session:
        yield session
