"""Fixtures for grass.tools"""

import os

import pytest

import grass.script as gs
from grass.tools.support import ToolResult, ToolFunctionResolver


@pytest.fixture
def xy_dataset_session(tmp_path):
    """Creates a session with XY project"""
    gs.create_project(tmp_path / "test")
    with gs.setup.init(tmp_path / "test", env=os.environ.copy()) as session:
        yield session


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
