# SPDX-License-Identifier: GPL-2.0-or-later

"""Test that r.sim.water refuses to run in a latitude-longitude project"""

import os

import numpy as np
import pytest

import grass.script as gs
from grass.tools import Tools, ToolError


def test_latlong_project_is_rejected(tmp_path):
    """The tool fails instead of taking one degree as one meter"""
    project = tmp_path / "latlong"
    gs.create_project(project, epsg="4326")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=5, w=0, e=6, res=1)
        with pytest.raises(ToolError) as error:
            tools.r_sim_water(elevation=np.zeros((5, 6)), depth="depth")
    assert "Lat/Long project is not supported" in error.value.errors
    assert "projected coordinate system" in error.value.errors
