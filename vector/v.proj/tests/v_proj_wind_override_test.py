# SPDX-License-Identifier: GPL-2.0-or-later

"""Regression test for v.proj run with a region override set."""

import os
from io import StringIO

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def source_and_target(tmp_path):
    """Two projects with different CRS.

    The source (lon/lat) holds a vector map in PERMANENT; the target
    (NC State Plane, meters) is where v.proj runs.
    """
    source = tmp_path / "source"
    target = tmp_path / "target"
    gs.create_project(source, epsg="4326")
    gs.create_project(target, epsg="3358")
    with gs.setup.init(source, env=os.environ.copy()) as session:
        Tools(session=session).v_in_ascii(
            input=StringIO("-78.6|35.7\n-78.5|35.8\n"),
            output="points",
            separator="|",
            format="point",
        )
    return source, target


def test_vproj_succeeds_with_wind_override(source_and_target):
    """v.proj imports a map while WIND_OVERRIDE names a region only in the
    target mapset."""
    source, target = source_and_target
    with gs.setup.init(target, env=os.environ.copy()) as session:
        # A named region that exists only in the target's current mapset.
        tools = Tools(session=session)
        tools.g_region(
            n=300000,
            s=200000,
            e=700000,
            w=600000,
            res=10,
            save="local_region",
        )
        # Set the override after init so it is not stripped by runtime setup;
        # this mirrors a caller running under a temporary named region.
        session.env["WIND_OVERRIDE"] = "local_region"
        tools.v_proj(
            project=source.name,
            mapset="PERMANENT",
            input="points",
            output="points",
        )
        assert tools.g_findfile(file="points", element="vector", format="json")["name"]
