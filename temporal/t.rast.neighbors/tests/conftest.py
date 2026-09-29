# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Fixture for t.rast.neighbors tests"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session_with_strds(tmp_path):
    """Start a session in a new project with a space time raster dataset."""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=3, w=0, e=3, res=1)
        for name, expression in (
            ("jan", "row()"),
            ("feb", "row()"),
            ("mar", "null()"),
        ):
            tools.r_mapcalc(expression=f"{name} = {expression}")
        tools.r_semantic_label(map="jan", semantic_label="S1")
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="input",
            title="Input",
            description="Input dataset",
        )
        tools.t_register(
            flags="i",
            type="raster",
            input="input",
            maps="jan,feb,mar",
            start="2001-01-01",
            increment="1 month",
        )
        yield session
