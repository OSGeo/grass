"""Test current baseline functionality of t.connect"""

from grass.tools import Tools


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

    result = tools.t_connect(flags="pg")
    lines = result.text_split("\n")

    assert "driver=sqlite" in lines
    assert "database=$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db" in lines
