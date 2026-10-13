"""Test current baseline functionality of t.connect"""

import pathlib

import pytest

import grass.script as gs
from grass.tools import ToolError, Tools


def test_no_connection(empty_session):
    """Check that printing an uninitialized connection returns empty fields."""
    tools = Tools(session=empty_session.session)
    result = tools.t_connect(flags="pg")

    lines = result.text_split("\n")

    assert lines == ["driver=", "database="]


def test_print_plain(t_connect_session):
    """Check that the -p flag prints the plain text layout."""
    tools = Tools(session=t_connect_session.session)

    result = tools.t_connect(flags="p")
    lines = result.text_split("\n")

    assert lines[0] == "driver:sqlite"
    assert lines[1] == "database:$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db"


def test_print_shell(t_connect_session):
    """Check that the shell format prints correctly."""
    tools = Tools(session=t_connect_session.session)

    result = tools.t_connect(flags="p", format="shell")
    lines = result.text_split("\n")

    assert "driver=sqlite" in lines
    assert "database=$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db" in lines


def test_print_json(t_connect_session):
    """Check that format=json returns an array with one entry per search-path mapset."""
    tools = Tools(session=t_connect_session.session)

    result = tools.t_connect(flags="p", format="json")

    assert len(result) == 1
    assert result[0]["mapset"] == "PERMANENT"
    assert result[0]["driver"] == "sqlite"
    assert result[0]["database"] == "$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db"


@pytest.mark.parametrize("output_format", ["shell", "json"])
def test_missing_print_flag(t_connect_session, output_format):
    """Ensure omitting the -p flag when requesting a format triggers a fatal error."""
    tools = Tools(session=t_connect_session.session)

    with pytest.raises(ToolError):
        tools.t_connect(format=output_format)


def test_incompatible_flags(t_connect_session):
    """Ensure combining the deprecated -g flag with format=json triggers a fatal error."""
    tools = Tools(session=t_connect_session.session)

    with pytest.raises(ToolError):
        tools.t_connect(flags="g", format="json")


def test_json_search_path_default(multi_mapset_session):
    """Check that format=json without mapset reports all search-path mapsets.

    The session is in user1, whose default search path includes PERMANENT,
    so both mapsets should appear in the output.
    """
    tools = Tools(session=multi_mapset_session.session)

    result = tools.t_connect(flags="p", format="json")
    mapsets = [entry["mapset"] for entry in result]

    assert "PERMANENT" in mapsets
    assert "user1" in mapsets


def test_json_mapset_dot_restricts_to_current(multi_mapset_session):
    """Check that mapset=. limits JSON output to the current mapset only."""
    tools = Tools(session=multi_mapset_session.session)

    result = tools.t_connect(flags="p", format="json", mapset=".")

    assert len(result) == 1
    assert result[0]["mapset"] == "user1"


def test_json_mapset_explicit_name(t_connect_session):
    """Check that an explicit mapset name returns exactly that mapset."""
    tools = Tools(session=t_connect_session.session)

    result = tools.t_connect(flags="p", format="json", mapset="PERMANENT")

    assert len(result) == 1
    assert result[0]["mapset"] == "PERMANENT"
    assert result[0]["driver"] == "sqlite"


def test_json_mapset_star_includes_all(multi_mapset_session):
    """Check that mapset=* reports connections for every mapset in the project."""
    tools = Tools(session=multi_mapset_session.session)

    result = tools.t_connect(flags="p", format="json", mapset="*")
    mapsets = {entry["mapset"] for entry in result}

    assert "PERMANENT" in mapsets
    assert "user1" in mapsets


def test_mapset_requires_json_format(t_connect_session):
    """Check that combining the mapset option with a non-JSON format raises an error."""
    tools = Tools(session=t_connect_session.session)

    with pytest.raises(ToolError):
        tools.t_connect(flags="p", format="shell", mapset=".")


def test_mapset_requires_print_flag(t_connect_session):
    """Check that using the mapset option without -p raises an error."""
    tools = Tools(session=t_connect_session.session)

    with pytest.raises(ToolError):
        tools.t_connect(mapset=".", format="json")


def test_print_does_not_write_var_file(empty_session):
    """Verify that print-only operations leave no TGIS entries in the VAR file.

    Calling t.connect with -p (in any output format) must never write
    connection settings to the mapset's VAR file.
    """
    tools = Tools(session=empty_session.session)

    tools.t_connect(flags="p")
    tools.t_connect(flags="p", format="shell")
    tools.t_connect(flags="p", format="json")

    gisenv = gs.gisenv(env=empty_session.session.env)
    var_file = (
        pathlib.Path(gisenv["GISDBASE"])
        / gisenv["LOCATION_NAME"]
        / gisenv["MAPSET"]
        / "VAR"
    )

    if var_file.exists():
        contents = var_file.read_text()
        assert "TGISDB_DRIVER" not in contents
        assert "TGISDB_DATABASE" not in contents
