"""Fixtures for grass.tools"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.tools.support import ToolResult, ToolFunctionResolver


@pytest.fixture
def xy_dataset_session(tmp_path):
    """Creates a session with XY project"""
    gs.create_project(tmp_path / "test")
    with gs.setup.init(tmp_path / "test", env=os.environ.copy()) as session:
        yield session


@pytest.fixture(scope="module")
def xy_dataset_session_for_module(tmp_path_factory):
    """Creates a session with XY project"""
    tmp_path = tmp_path_factory.mktemp("module_project")
    gs.create_project(tmp_path / "test")
    with gs.setup.init(tmp_path / "test", env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def plain_tools(xy_dataset_session_for_module):
    with Tools(session=xy_dataset_session_for_module) as tools:
        yield tools


@pytest.fixture
def empty_result():
    return ToolResult(
        name=None, command=None, kwargs=None, returncode=None, stdout=None, stderr=None
    )


@pytest.fixture
def empty_string_result():
    return ToolResult(
        name=None, command=None, kwargs=None, returncode=None, stdout="", stderr=""
    )


@pytest.fixture
def echoing_resolver():
    return ToolFunctionResolver(run_function=lambda x: x, env=os.environ.copy())
