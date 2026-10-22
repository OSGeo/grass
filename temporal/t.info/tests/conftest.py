# SPDX-License-Identifier: GPL-2.0-or-later
"""Fixture for t.info tests

Creates one project holding every dataset type t.info can report on, so that
the reported metadata can be compared against known values.
"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.grassdb.create import create_mapset
from grass.tools import Tools

MAPSET = "tinfo_mapset"


@pytest.fixture(scope="session")
def temporal_data(tmp_path_factory):
    """Start a session with space time datasets of all types and their maps"""
    tmp_path = tmp_path_factory.mktemp("t_info")
    project = tmp_path / "test_project"
    gs.create_project(project)
    create_mapset(project / MAPSET)

    with gs.setup.init(project / MAPSET, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10, tbres=10)

        # Maps with constant values keep the min and max metadata predictable.
        for i in range(1, 4):
            tools.r_mapcalc(expression=f"prec_{i} = {i}")
        for i in range(1, 3):
            tools.r3_mapcalc(expression=f"vol_{i} = {i}")
            tools.v_random(output=f"vect_{i}", npoints=20, seed=i)
            tools.r_mapcalc(expression=f"rel_{i} = {i}")

        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
        )
        tools.t_register(
            type="raster",
            flags="i",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3",
            start="2001-01-01",
            increment="1 months",
        )

        # A map can be registered in more than one dataset, which t.info
        # reports for the map.
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="precip_abs2",
            title="A second test",
            description="A second test",
        )
        tools.t_register(
            type="raster",
            input="precip_abs2",
            maps="prec_2",
        )

        tools.t_create(
            type="str3ds",
            temporaltype="absolute",
            output="volume_abs1",
            title="A 3D test",
            description="A 3D test",
        )
        tools.t_register(
            type="raster_3d",
            flags="i",
            input="volume_abs1",
            maps="vol_1,vol_2",
            start="2001-01-01",
            increment="1 months",
        )

        tools.t_create(
            type="stvds",
            temporaltype="absolute",
            output="vect_abs1",
            title="A vector test",
            description="A vector test",
        )
        tools.t_register(
            type="vector",
            flags="i",
            input="vect_abs1",
            maps="vect_1,vect_2",
            start="2001-01-01",
            increment="1 months",
        )

        # Relative time datasets need their own maps because a map can hold
        # only one time stamp.
        tools.t_create(
            type="strds",
            temporaltype="relative",
            output="precip_rel1",
            title="A relative test",
            description="A relative test",
        )
        tools.t_register(
            type="raster",
            flags="i",
            input="precip_rel1",
            maps="rel_1,rel_2",
            start=1,
            increment=1,
            unit="days",
        )

        # A dataset without registered maps reports empty metadata fields.
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="empty_ds",
            title="An empty test",
            description="An empty test",
        )

        # Non-ASCII text has to survive the round trip through the database.
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="accent_ds",
            title="Précipitation totale",
            description="Précipitation mensuelle",
        )

        yield SimpleNamespace(session=session, mapset=MAPSET, tools=tools)
