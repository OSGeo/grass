# Author: Vaclav Petras (2025)
# SPDX-License-Identifier: GPL-2.0-or-later

"""pytest test fixtures for raster tools"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.experimental import TemporaryMapsetSession


@pytest.fixture(scope="module")
def session_with_raster_for_module(tmp_path_factory):
    """Active session with CRS and raster data"""
    project = tmp_path_factory.mktemp("scripts_raster_dataset") / "xy_test"
    gs.create_project(project, epsg=3358)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=5, w=0, e=6, res=1, flags="s")
        tools.r_mapcalc(expression="rows_raster = row()")
        yield session


@pytest.fixture
def session_in_mapset(session_with_raster_for_module):
    """A session in a temporary mapset with data in PERMANENT.

    See the fixture code for available data.
    """
    with TemporaryMapsetSession(env=session_with_raster_for_module.env) as session:
        yield session


@pytest.fixture
def session_tools(session_in_mapset):
    """A Tools object ready to be used.

    See the underlying fixture for more info.
    """
    with Tools(session=session_in_mapset) as tools:
        yield tools
