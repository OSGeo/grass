# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Fixtures for t.rast.algebra tests"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session_with_relative_strds(tmp_path):
    """Start a session with a dataset of three maps one day long in relative time."""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=2, w=0, e=2, res=1)
        for value in (1, 2, 3):
            tools.r_mapcalc(expression=f"a{value} = {value}")
        tools.t_create(
            output="A",
            temporaltype="relative",
            title="Input",
            description="Input dataset",
        )
        tools.t_register(
            input="A", maps="a1,a2,a3", start=0, increment=1, unit="days", flags="i"
        )
        yield session
