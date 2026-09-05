# SPDX-License-Identifier: GPL-2.0-or-later
import os

import pytest

import grass.script as gs
from grass.tools import Tools

NPOINTS = 100


@pytest.fixture
def session(tmp_path):
    """A GRASS session with a synthetic area smaller than the region.

    The area (the inner part of the region) is used to test the restrict=
    option without depending on any sample dataset.
    """
    project = tmp_path / "v_random_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=100, s=0, e=100, w=0, res=1)
        tools.g_region(n=80, s=20, e=80, w=20)
        tools.v_in_region(output="restrict_area")
        tools.g_region(n=100, s=0, e=100, w=0, res=1)
        yield session


def test_number_of_points(session):
    """v.random creates exactly the requested number of points."""
    tools = Tools(session=session)
    tools.v_random(output="points", npoints=NPOINTS)

    topology = tools.v_info(map="points", flags="t", format="json").json
    assert topology["points"] == NPOINTS


def test_points_are_3d(session):
    """With zmin/zmax and the -z flag the output is a 3D point map."""
    tools = Tools(session=session)
    tools.v_random(output="points", npoints=NPOINTS, zmin=10, zmax=120, flags="z")

    topology = tools.v_info(map="points", flags="t", format="json")
    assert topology["points"] == NPOINTS
    assert topology["map3d"] == 1


def test_restrict_to_area(session):
    """restrict= keeps every point inside the area, so clipping removes none."""
    tools = Tools(session=session)
    tools.v_random(output="points", npoints=NPOINTS, restrict="restrict_area")
    tools.v_clip(input="points", clip="restrict_area", output="clipped")

    points = tools.v_info(map="points", flags="t", format="json").json
    clipped = tools.v_info(map="clipped", flags="t", format="json").json
    assert points["points"] == clipped["points"] == NPOINTS
