# SPDX-FileCopyrightText: 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Fixtures for t.unregister tests"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session_with_strds(tmp_path):
    """Session with rasters a1..a6 and STRDS A and B, each with all maps registered."""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=1, w=0, e=1, res=1)
        for i in range(1, 7):
            tools.r_mapcalc(expression=f"a{i} = {i * 100}")
        maps = ",".join(f"a{i}" for i in range(1, 7))
        for name in ("A", "B"):
            tools.t_create(
                type="strds",
                temporaltype="absolute",
                output=name,
                title=f"{name} test",
                description=f"{name} test",
            )
            tools.t_register(
                flags="i",
                type="raster",
                input=name,
                maps=maps,
                start="2001-01-01",
                increment="3 months",
            )
        yield session
