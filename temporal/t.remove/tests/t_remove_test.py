# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for t.remove

Covers the preview (no force) case, the three levels of actual removal
(dataset only, dataset and unregistering, dataset and deleting the maps),
the cross-dataset effect of fully deleting a shared map, multiple datasets
given as a comma-separated list or a file, and the input validation rules.
"""

import pytest

from grass.tools import ToolError, Tools


def _create_strds(tools, name, maps=(), start="2001-01-01", increment="1 month"):
    """Create an absolute-time STRDS, optionally registering new rasters in it."""
    tools.t_create(
        type="strds",
        temporaltype="absolute",
        output=name,
        title=name,
        description=name,
    )
    if maps:
        tools.t_register(
            type="raster",
            input=name,
            maps=",".join(maps),
            start=start,
            increment=increment,
            flags="i",
        )


def _strds_names(tools):
    """Return the names of all STRDS in the temporal database."""
    return [row["name"] for row in tools.t_list(type="strds", format="json").json]


def _raster_names_in_tgis_db(tools):
    """Return the names of all raster maps registered in the temporal database."""
    return [row["name"] for row in tools.t_list(type="raster", format="json").json]


def _map_names(tools, type_):
    """Return the names of all maps of the given type in the spatial database."""
    return [row["name"] for row in tools.g_list(type=type_, format="json").json]


def test_without_force_leaves_everything_unchanged(remove_session):
    """Without -f the tool only previews the removal; nothing is changed."""
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="preview_1 = 1")
    _create_strds(tools, "preview_strds", maps=["preview_1"])

    tools.t_remove(type="strds", inputs="preview_strds")

    assert "preview_strds" in _strds_names(tools)
    assert "preview_1" in _raster_names_in_tgis_db(tools)
    assert "preview_1" in _map_names(tools, "raster")


def test_force_only_removes_dataset_but_keeps_maps(remove_session):
    """-f alone removes the STDS entry but leaves the maps registered and in place."""
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="force_only_1 = 1")
    _create_strds(tools, "force_only_strds", maps=["force_only_1"])

    tools.t_remove(type="strds", inputs="force_only_strds", flags="f")

    assert "force_only_strds" not in _strds_names(tools)
    assert "force_only_1" in _raster_names_in_tgis_db(tools)
    assert "force_only_1" in _map_names(tools, "raster")


def test_recursive_force_unregisters_maps_but_keeps_files(remove_session):
    """-rf removes the STDS and unregisters its maps, but keeps the map files."""
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="recursive_1 = 1")
    _create_strds(tools, "recursive_strds", maps=["recursive_1"])

    tools.t_remove(type="strds", inputs="recursive_strds", flags="rf")

    assert "recursive_strds" not in _strds_names(tools)
    assert "recursive_1" not in _raster_names_in_tgis_db(tools)
    assert "recursive_1" in _map_names(tools, "raster")


def test_delete_removes_the_raster_files_too(remove_session):
    """-df removes the STDS, unregisters the maps and deletes the raster files."""
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="delete_1 = 1")
    _create_strds(tools, "delete_strds", maps=["delete_1"])

    tools.t_remove(type="strds", inputs="delete_strds", flags="df")

    assert "delete_strds" not in _strds_names(tools)
    assert "delete_1" not in _raster_names_in_tgis_db(tools)
    assert "delete_1" not in _map_names(tools, "raster")


def test_delete_removes_vector_files_for_stvds(remove_session):
    """-df on a STVDS deletes the vector maps, exercising the vector g.remove path."""
    tools = Tools(session=remove_session)
    tools.v_random(output="delete_vect_1", npoints=5, seed=1)
    tools.t_create(
        type="stvds",
        temporaltype="absolute",
        output="delete_vstrds",
        title="delete_vstrds",
        description="delete_vstrds",
    )
    tools.t_register(
        type="vector",
        input="delete_vstrds",
        maps="delete_vect_1",
        start="2001-01-01",
        increment="1 month",
        flags="i",
    )

    tools.t_remove(type="stvds", inputs="delete_vstrds", flags="df")

    assert "delete_vstrds" not in [
        row["name"] for row in tools.t_list(type="stvds", format="json").json
    ]
    assert "delete_vect_1" not in _map_names(tools, "vector")


def test_recursive_force_on_shared_map_affects_other_dataset(remove_session):
    """Fully deleting a map registered in several STDS removes it from all of them.

    t.remove -r deletes each of its input STDS's maps from the temporal
    database entirely (not just from that one STDS), so a map shared with
    another, untouched STDS disappears from there too.
    """
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="shared_only_a = 1")
    tools.r_mapcalc(expression="shared_map = 2")
    tools.r_mapcalc(expression="shared_only_b = 3")
    _create_strds(tools, "shared_strds_a", maps=["shared_only_a", "shared_map"])
    _create_strds(tools, "shared_strds_b", maps=["shared_only_b"])
    # shared_map already has a time stamp from strds_a, so it is registered
    # in strds_b without another start/increment.
    tools.t_register(type="raster", input="shared_strds_b", maps="shared_map")

    tools.t_remove(type="strds", inputs="shared_strds_a", flags="rf")

    assert "shared_strds_a" not in _strds_names(tools)
    remaining = [
        row["name"]
        for row in tools.t_rast_list(
            input="shared_strds_b", columns="name", format="json"
        ).json["data"]
    ]
    assert remaining == ["shared_only_b"]


def test_cannot_remove_foreign_mapset_stds_without_local_db(remove_session):
    """A STDS in another mapset is left alone when the current mapset has no
    temporal database at all: t.remove finds no database to query and quietly
    does nothing, instead of falling back to some other mapset's database.
    """
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="foreign_nodb_1 = 1")
    _create_strds(tools, "foreign_nodb_strds", maps=["foreign_nodb_1"])

    tools.g_mapset(mapset="foreign_nodb", flags="c")
    try:
        tools.t_remove(type="strds", inputs="foreign_nodb_strds@PERMANENT", flags="df")
    finally:
        tools.g_mapset(mapset="PERMANENT")

    assert "foreign_nodb_strds" in _strds_names(tools)
    assert "foreign_nodb_1" in _map_names(tools, "raster")


def test_cannot_remove_foreign_mapset_stds_with_local_db(remove_session):
    """A STDS in another mapset is left alone even when the current mapset
    has its own temporal database: t.remove connects only to the current
    mapset's database, so the STDS named as name@PERMANENT is not found
    there and the removal fails instead of reaching into PERMANENT.
    """
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="foreign_withdb_1 = 1")
    _create_strds(tools, "foreign_withdb_strds", maps=["foreign_withdb_1"])

    tools.g_mapset(mapset="foreign_withdb", flags="c")
    try:
        # Force a local temporal database to exist in this mapset.
        _create_strds(tools, "local")
        with pytest.raises(ToolError):
            tools.t_remove(
                type="strds", inputs="foreign_withdb_strds@PERMANENT", flags="df"
            )
    finally:
        tools.g_mapset(mapset="PERMANENT")

    assert "foreign_withdb_strds" in _strds_names(tools)
    assert "foreign_withdb_1" in _map_names(tools, "raster")


def test_removing_an_already_removed_dataset_fails(remove_session):
    """Removing a STDS that no longer exists reports an error instead of succeeding."""
    tools = Tools(session=remove_session)
    tools.r_mapcalc(expression="twice_1 = 1")
    _create_strds(tools, "twice_strds", maps=["twice_1"])

    tools.t_remove(type="strds", inputs="twice_strds", flags="rf")
    with pytest.raises(ToolError):
        tools.t_remove(type="strds", inputs="twice_strds", flags="r")


def test_multiple_datasets_via_comma_separated_inputs(remove_session):
    """A single call with a comma-separated list removes every named STDS."""
    tools = Tools(session=remove_session)
    _create_strds(tools, "multi_1")
    _create_strds(tools, "multi_2")

    tools.t_remove(type="strds", inputs="multi_1,multi_2", flags="f")

    names = _strds_names(tools)
    assert "multi_1" not in names
    assert "multi_2" not in names


def test_datasets_via_file_option(remove_session, tmp_path):
    """The file option accepts dataset names listed one per line."""
    tools = Tools(session=remove_session)
    _create_strds(tools, "file_listed_1")
    _create_strds(tools, "file_listed_2")
    dataset_file = tmp_path / "names.txt"
    dataset_file.write_text("file_listed_1\nfile_listed_2\n")

    tools.t_remove(type="strds", file=dataset_file, flags="f")

    names = _strds_names(tools)
    assert "file_listed_1" not in names
    assert "file_listed_2" not in names


def test_inputs_and_file_are_mutually_exclusive(remove_session, tmp_path):
    """inputs and file cannot be given together; the STDS is left untouched."""
    tools = Tools(session=remove_session)
    _create_strds(tools, "exclusive_ds")
    dataset_file = tmp_path / "names.txt"
    dataset_file.write_text("exclusive_ds\n")

    with pytest.raises(ToolError):
        tools.t_remove(
            type="strds", inputs="exclusive_ds", file=dataset_file, flags="f"
        )

    assert "exclusive_ds" in _strds_names(tools)
