"""Test current baseline functionality of t.connect"""

import pytest

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
    """Check that the JSON format parses correctly and matches connection values."""
    tools = Tools(session=t_connect_session.session)

    result = tools.t_connect(flags="p", format="json")

    assert result["driver"] == "sqlite"
    assert result["database"] == "$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db"


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
