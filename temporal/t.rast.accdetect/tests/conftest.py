# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Fixtures for t.rast.accdetect tests"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session_with_strds(tmp_path):
    """Start a session with a dataset of one daily map and maps to use as limits."""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        for name, value in (("input_map", 20), ("minimum", 30), ("maximum", 10)):
            tools.r_mapcalc(expression=f"{name} = {value}")
        tools.t_create(output="input", title="Input", description="Input dataset")
        tools.t_register(
            input="input", maps="input_map", start="2001-01-01", end="2001-01-02"
        )
        yield session
