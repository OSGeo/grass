"""Tests for grass.script.imagery group_to_dict()"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def group_session(tmp_path_factory):
    """Session with a group of three labeled rasters, one unlabeled raster, and a subgroup."""
    tmp_path = tmp_path_factory.mktemp("imagery_group_test")
    project = tmp_path / "test_project"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(rows=2, cols=2)
        for name in ("band_1", "band_2", "band_3", "band_unlabeled"):
            tools.r_mapcalc(expression=f"{name} = 1")
        for name, label in (
            ("band_1", "L8_1"),
            ("band_2", "L8_2"),
            ("band_3", "L8_3"),
        ):
            tools.r_support(map=name, semantic_label=label)
        tools.i_group(group="test_group", input="band_1,band_2,band_3,band_unlabeled")
        tools.i_group(
            group="test_group", subgroup="test_subgroup", input="band_1,band_2"
        )
        yield session


@pytest.fixture(scope="module")
def mapset(group_session):
    """Mapset holding the test rasters, i.e. the suffix of their fully qualified names."""
    return gs.gisenv(env=group_session.env)["MAPSET"]


@pytest.fixture(scope="module")
def cross_mapset_group_session(tmp_path_factory):
    """Session with a group in another mapset that is on the search path but not current."""
    tmp_path = tmp_path_factory.mktemp("imagery_cross_mapset_test")
    project = tmp_path / "test_project"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_mapset(flags="c", mapset="other_mapset")
        tools.g_region(rows=2, cols=2)
        tools.r_mapcalc(expression="other_band = 1")
        tools.i_group(group="other_group", input="other_band")
        tools.g_mapset(mapset="PERMANENT")
        tools.g_mapsets(mapset="other_mapset", operation="add")
        yield session


def test_default_keys_and_values(group_session, mapset):
    """Semantic labels are the default keys, map names the default values.

    The unlabeled raster has no semantic label, so its 1-based index in the
    group is used as a fallback key (fill_semantic_label defaults to True).
    """
    group_dict = gs.group_to_dict("test_group", env=group_session.env)
    assert group_dict == {
        "L8_1": f"band_1@{mapset}",
        "L8_2": f"band_2@{mapset}",
        "L8_3": f"band_3@{mapset}",
        "4": f"band_unlabeled@{mapset}",
    }


def test_indices_as_keys(group_session, mapset):
    """Index keys follow the order maps were added to the group, starting at 1."""
    group_dict = gs.group_to_dict(
        "test_group",
        dict_keys="indices",
        dict_values="map_names",
        env=group_session.env,
    )
    assert group_dict == {
        "1": f"band_1@{mapset}",
        "2": f"band_2@{mapset}",
        "3": f"band_3@{mapset}",
        "4": f"band_unlabeled@{mapset}",
    }


def test_map_names_as_keys_and_semantic_labels_as_values(group_session, mapset):
    """Map names as keys, semantic labels as values.

    r.info reports a missing semantic label as the literal string '"none"'
    (with quotes), so that is what an unlabeled map's value looks like here.
    """
    group_dict = gs.group_to_dict(
        "test_group",
        dict_keys="map_names",
        dict_values="semantic_labels",
        env=group_session.env,
    )
    assert group_dict == {
        f"band_1@{mapset}": "L8_1",
        f"band_2@{mapset}": "L8_2",
        f"band_3@{mapset}": "L8_3",
        f"band_unlabeled@{mapset}": '"none"',
    }


def test_metadata_values(group_session):
    """dict_values='metadata' returns full raster_info() dictionaries as values."""
    group_dict = gs.group_to_dict(
        "test_group", dict_values="metadata", env=group_session.env
    )
    assert group_dict["L8_1"]["rows"] == "2"
    assert group_dict["L8_1"]["cols"] == "2"


def test_subgroup_restricts_to_its_maps(group_session, mapset):
    """Only maps belonging to the given subgroup are returned."""
    group_dict = gs.group_to_dict(
        "test_group", subgroup="test_subgroup", env=group_session.env
    )
    assert group_dict == {
        "L8_1": f"band_1@{mapset}",
        "L8_2": f"band_2@{mapset}",
    }


def test_missing_subgroup_warns_and_returns_empty_dict(group_session, capfd):
    """A non-existent (or empty) subgroup produces a warning and an empty dict.

    This follows the behavior of i.group itself, which treats such a
    subgroup as empty rather than as an error.
    """
    group_dict = gs.group_to_dict(
        "test_group", subgroup="no_such_subgroup", env=group_session.env
    )
    assert group_dict == {}
    assert "no_such_subgroup" in capfd.readouterr().err


def test_unlabeled_map_is_fatal_without_fallback(group_session):
    """fill_semantic_label=False raises a fatal error for an unlabeled map."""
    with pytest.raises(SystemExit):
        gs.group_to_dict("test_group", fill_semantic_label=False, env=group_session.env)


@pytest.mark.parametrize("bad_option", ["dict_keys", "dict_values"])
def test_invalid_option_raises_value_error(group_session, bad_option):
    """An unsupported dict_keys or dict_values value raises ValueError."""
    with pytest.raises(ValueError, match="Invalid dictionary"):
        gs.group_to_dict(
            "test_group", env=group_session.env, **{bad_option: "not_a_real_option"}
        )


def test_unqualified_group_in_other_mapset_is_resolved(cross_mapset_group_session):
    """An unqualified group name is resolved via the mapset search path.

    i.group itself only resolves an unqualified group name in the current
    mapset, so group_to_dict() has to qualify it first to find a group that
    lives in another, merely accessible, mapset.
    """
    group_dict = gs.group_to_dict(
        "other_group", dict_keys="indices", env=cross_mapset_group_session.env
    )
    assert group_dict == {"1": "other_band@other_mapset"}
