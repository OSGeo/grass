# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Fixture for t.register tests"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session_with_rasters(tmp_path):
    """Start a session in a new project and create small test rasters."""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=1, w=0, e=1, res=1)
        for name in ("a", "b", "c"):
            tools.r_mapcalc(expression=f"{name} = 1")
        yield session
