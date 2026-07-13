"""General pytest test fixtures

Author: Edouard Choinière (2025-03-09)
SPDX-License-Identifier: GPL-2.0-or-later
"""

import os
from collections.abc import Generator

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.experimental import TemporaryMapsetSession


@pytest.fixture(scope="module")
def monkeypatch_module() -> Generator[pytest.MonkeyPatch]:
    """Yield a monkeypatch context, through a module-scoped fixture"""
    with pytest.MonkeyPatch.context() as mp:
        yield mp


@pytest.fixture(scope="module")
def xy_raster_dataset_session_for_module(tmp_path_factory):
    """Active session in an XY location with small data (scope: function)"""
    project = tmp_path_factory.mktemp("scripts_raster_dataset") / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=5, w=0, e=6, res=1, flags="s")
        tools.r_mapcalc(expression="rows_raster = row()")
        yield session


@pytest.fixture
def xy_raster_dataset_session_mapset(xy_raster_dataset_session_for_module):
    """A session in a temporary mapset"""
    with TemporaryMapsetSession(
        env=xy_raster_dataset_session_for_module.env
    ) as session:
        yield session


@pytest.fixture
def xy_empty_dataset_session(tmp_path):
    """Active session in an XY location (scope: function)"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=5, w=0, e=6, res=1)
        yield session
