# SPDX-FileCopyrightText: 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests for t.unregister

Covers unregistering raster maps from a specific STRDS, from the temporal
database entirely, verifying that maps survive STRDS removal, and that
maps in another mapset's temporal database cannot be unregistered.
"""

import pytest

from grass.tools import ToolError, Tools

ALL_MAPS = ["a1", "a2", "a3", "a4", "a5", "a6"]
FIRST_MAPS = ALL_MAPS[:3]
LAST_MAPS = ALL_MAPS[3:]


def strds_map_names(tools, strds):
    """Return names of maps registered in the given STRDS, in temporal order."""
    return [
        row["name"]
        for row in tools.t_rast_list(input=strds, columns="name", format="json").json[
            "data"
        ]
    ]


def db_map_names(tools, names):
    """Return names of the given maps that are present in the temporal database."""
    where = " OR ".join(f"name = '{name}'" for name in names)
    return [
        row["name"]
        for row in tools.t_list(
            type="raster", columns="name", format="json", where=where
        ).json
    ]


def test_unregister_from_strds_keeps_maps_elsewhere(session_with_strds):
    """Unregistering maps from one STRDS leaves them in other STRDS and the DB."""
    tools = Tools(session=session_with_strds)

    tools.t_unregister(input="A", type="raster", maps=",".join(FIRST_MAPS))

    assert strds_map_names(tools, "A") == LAST_MAPS
    assert strds_map_names(tools, "B") == ALL_MAPS
    assert db_map_names(tools, FIRST_MAPS) == FIRST_MAPS


def test_unregister_from_db_removes_maps_everywhere(session_with_strds):
    """Unregistering maps from the DB removes them from all STRDS."""
    tools = Tools(session=session_with_strds)

    tools.t_unregister(input="A", type="raster", maps=",".join(FIRST_MAPS))
    tools.t_unregister(type="raster", maps=",".join(FIRST_MAPS))

    assert strds_map_names(tools, "A") == LAST_MAPS
    assert strds_map_names(tools, "B") == LAST_MAPS
    assert db_map_names(tools, FIRST_MAPS) == []
    assert db_map_names(tools, LAST_MAPS) == LAST_MAPS


def test_maps_remain_in_db_after_strds_removal(session_with_strds):
    """Maps remain in the temporal DB when their STRDS are removed."""
    tools = Tools(session=session_with_strds)

    tools.t_unregister(type="raster", maps=",".join(FIRST_MAPS))
    tools.t_remove(type="strds", inputs="A,B")

    assert db_map_names(tools, LAST_MAPS) == LAST_MAPS

    tools.t_unregister(type="raster", maps=",".join(LAST_MAPS))

    assert db_map_names(tools, LAST_MAPS) == []


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
    assert strds_map_names(tools, "A") == ALL_MAPS


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
    assert strds_map_names(tools, "A") == ALL_MAPS
