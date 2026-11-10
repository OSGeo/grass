# SPDX-FileCopyrightText: 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests for t.unregister

Covers unregistering raster maps from a specific STRDS, from the temporal
database entirely, verifying that maps survive STRDS removal, and that
maps in another mapset's temporal database cannot be unregistered.
"""

import pytest

from grass.tools import ToolError, Tools

ALL_MAPS = [f"a{i}@PERMANENT" for i in range(1, 7)]
FIRST_MAPS = ALL_MAPS[:3]
LAST_MAPS = ALL_MAPS[3:]


def strds_map_ids(tools, strds):
    """Return IDs of maps registered in the given STRDS, in temporal order."""
    return [
        row["id"]
        for row in tools.t_rast_list(input=strds, columns="id", format="json").json[
            "data"
        ]
    ]


def db_map_ids(tools, ids):
    """Return IDs of the given maps that are present in the temporal database."""
    where = " OR ".join(f"id = '{map_id}'" for map_id in ids)
    return [
        row["id"]
        for row in tools.t_list(
            type="raster", columns="id", format="json", where=where
        ).json
    ]


def strds_map_count(tools, strds):
    """Return the number of maps recorded in the STRDS metadata."""
    info = tools.t_info(flags="g", type="strds", input=strds).keyval
    return int(info["number_of_maps"])


def test_unregister_from_strds_keeps_maps_elsewhere(session_with_strds):
    """Unregistering maps from one STRDS leaves them in other STRDS and the DB."""
    tools = Tools(session=session_with_strds)

    tools.t_unregister(input="A", type="raster", maps=",".join(FIRST_MAPS))

    assert strds_map_ids(tools, "A") == LAST_MAPS
    assert strds_map_ids(tools, "B") == ALL_MAPS
    assert db_map_ids(tools, FIRST_MAPS) == FIRST_MAPS


def test_unregister_from_db_removes_maps_everywhere(session_with_strds):
    """Unregistering maps from the DB removes them from all STRDS."""
    tools = Tools(session=session_with_strds)

    tools.t_unregister(input="A", type="raster", maps=",".join(FIRST_MAPS))
    tools.t_unregister(type="raster", maps=",".join(FIRST_MAPS))

    assert strds_map_ids(tools, "A") == LAST_MAPS
    assert strds_map_ids(tools, "B") == LAST_MAPS
    assert db_map_ids(tools, FIRST_MAPS) == []
    assert db_map_ids(tools, LAST_MAPS) == LAST_MAPS


def test_unregister_from_db_updates_all_affected_strds(session_with_strds):
    """Metadata of every STRDS holding one of the maps is updated."""
    tools = Tools(session=session_with_strds)
    # a1 remains only in A and a2 only in B.
    tools.t_unregister(input="B", type="raster", maps="a1")
    tools.t_unregister(input="A", type="raster", maps="a2")

    tools.t_unregister(type="raster", maps="a1,a2")

    assert strds_map_ids(tools, "A") == ALL_MAPS[2:]
    assert strds_map_ids(tools, "B") == ALL_MAPS[2:]
    assert strds_map_count(tools, "A") == len(ALL_MAPS) - 2
    assert strds_map_count(tools, "B") == len(ALL_MAPS) - 2


def test_maps_remain_in_db_after_strds_removal(session_with_strds):
    """Maps remain in the temporal DB when their STRDS are removed."""
    tools = Tools(session=session_with_strds)

    tools.t_unregister(type="raster", maps=",".join(FIRST_MAPS))
    tools.t_remove(type="strds", inputs="A,B")

    assert db_map_ids(tools, LAST_MAPS) == LAST_MAPS

    tools.t_unregister(type="raster", maps=",".join(LAST_MAPS))

    assert db_map_ids(tools, LAST_MAPS) == []


def test_cannot_unregister_from_foreign_mapset_strds(session_with_strds):
    """Unregistering from a STRDS in another mapset's temporal DB fails.

    The tool connects to the current mapset's temporal DB. A STRDS that
    lives in a different mapset is not found there, so the tool exits
    with an error and leaves the foreign STRDS unchanged.
    """
    tools = Tools(session=session_with_strds)
    tools.g_mapset(mapset="other", flags="c")

    with pytest.raises(ToolError):
        tools.t_unregister(input="A@PERMANENT", type="raster", maps="a1")

    tools.g_mapset(mapset="PERMANENT")
    assert strds_map_ids(tools, "A") == ALL_MAPS


def test_cannot_unregister_from_foreign_mapset_strds_when_local_db_exists(
    session_with_strds,
):
    """Unregistering from a foreign STRDS fails even when the current mapset has its own DB.

    Creates an empty STRDS in a second mapset to ensure its temporal DB is
    initialised, then confirms that a STRDS owned by PERMANENT is still not
    accessible from there.
    """
    tools = Tools(session=session_with_strds)
    tools.g_mapset(mapset="other", flags="c")
    tools.t_create(
        type="strds",
        temporaltype="absolute",
        output="local",
        title="local",
        description="local",
    )

    with pytest.raises(ToolError):
        tools.t_unregister(input="A@PERMANENT", type="raster", maps="a1")

    tools.g_mapset(mapset="PERMANENT")
    assert strds_map_ids(tools, "A") == ALL_MAPS
