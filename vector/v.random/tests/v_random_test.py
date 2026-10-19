# SPDX-License-Identifier: GPL-2.0-or-later
import os

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
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

    topology = tools.v_info(map="points", flags="t", format="json")
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

    points = tools.v_info(map="points", flags="t", format="json")
    clipped = tools.v_info(map="clipped", flags="t", format="json")
    assert points["points"] == clipped["points"] == NPOINTS


def test_2d_points_for_seed(session):
    """The 2D points of a seed are the pinned ones"""
    tools = Tools(session=session)
    tools.v_random(output="points", npoints=3, seed=42)

    text = tools.v_out_ascii(
        input="points", format="point", precision=17, separator="comma"
    ).text
    values = [float(value) for line in text.split() for value in line.split(",")]
    assert values == pytest.approx(
        [
            *(25.547499993899336, 34.2701478718908, 1),
            *(57.7661042011691, 8.111117117831057, 2),
            *(50.12005778059212, 47.88142906446282, 3),
        ],
        rel=1e-15,
    )


def test_points_for_seed(session):
    """The 3D points of a seed are the pinned ones

    Pinned separately from the 2D points, since how the draws are ordered
    between the coordinates could change.
    """
    tools = Tools(session=session)
    tools.v_random(output="points", npoints=3, seed=42, zmin=10, zmax=120, flags="z")

    text = tools.v_out_ascii(
        input="points", format="point", precision=17, separator="comma"
    ).text
    values = [float(value) for line in text.split() for value in line.split(",")]
    assert values == pytest.approx(
        [
            *(25.547499993899336, 34.2701478718908, 22.219381068857764, 1),
            *(57.7661042011691, 8.111117117831057, 104.20847788292875, 2),
            *(50.12005778059212, 47.88142906446282, 85.98936887362026, 3),
        ],
        rel=1e-15,
    )


@pytest.mark.parametrize("seed", ["5000000000", "-2147483649", "12abc"])
def test_invalid_seed_refused(session, seed):
    """A seed out of range or not wholly an integer is an error"""
    tools = Tools(session=session)
    with pytest.raises(CalledModuleError):
        tools.v_random(output="points", npoints=NPOINTS, seed=seed)
    assert not tools.g_list(type="vector", pattern="points", format="json")
