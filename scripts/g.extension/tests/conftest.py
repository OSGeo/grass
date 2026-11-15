"""Test g.extension"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


# Using module-scoped fixture to avoid overhead of downloading addons repo.
@pytest.fixture(scope="module")
def tools(tmp_path_factory):
    """Tools with modified addon base directory (scope: module)"""
    tmp_path = tmp_path_factory.mktemp("g_extension_tests")
    gs.create_project(tmp_path / "test")
    env = os.environ.copy()
    env["GRASS_ADDON_BASE"] = str(tmp_path / "addons")
    with (
        gs.setup.init(tmp_path / "test", env=env) as session,
        Tools(session=session, consistent_return_value=True) as tools,
    ):
        yield tools
