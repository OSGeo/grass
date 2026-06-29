import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.tools import Tools


def test_db_connect_normal_database_name(session):
    """Test that db.connect works with normal database names."""
    tools = Tools(session=session)
    tools.db_connect(flags="d")
    output = tools.db_connect(flags="p", format="json")
    assert output["driver"] == "sqlite"
    assert "sqlite.db" in output["database"]


def test_db_connect_long_valid_database_name(session):
    """Test that db.connect works with long but valid database names."""
    long_name = "/tmp/" + "a" * 2000 + "/test.db"

    tools = Tools(session=session)
    try:
        tools.db_connect(driver="sqlite", database=long_name)
        output = gs.read_command("db.connect", flags="p", env=session.env)
        assert "test.db" in output
    except CalledModuleError as e:
        error_msg = str(e).lower()
        assert "too long" not in error_msg
        assert "exceeds" not in error_msg


def test_db_connect_overlong_database_name_rejected(session):
    """Test that printing a stored database name exceeding GPATH_MAX fails.

    Setting the connection accepts any string without validation. The overflow
    check fires in substitute_variables(), which is called only in print mode.
    """
    overlong_name = "/tmp/" + "x" * 4500 + "/test.db"

    tools = Tools(session=session)
    tools.db_connect(driver="sqlite", database=overlong_name)
    with pytest.raises(CalledModuleError, match=r"too long|exceeds.*characters"):
        tools.db_connect(flags="p")


def test_db_connect_variable_expansion_overflow(session):
    """Test that variable expansion is bounds-checked."""
    gisenv = gs.gisenv(env=session.env)
    gisdbase = gisenv["GISDBASE"]
    location = gisenv["LOCATION_NAME"]
    mapset = gisenv["MAPSET"]

    expanded_length = len(gisdbase) + len(location) + len(mapset) + 100
    padding_needed = 4096 - expanded_length + 500

    if padding_needed > 0:
        overlong_template = (
            "$GISDBASE/" + "x" * padding_needed + "/$LOCATION_NAME/$MAPSET/test.db"
        )

        # Setting the connection stores the template without expanding it.
        # The overflow check runs in substitute_variables(), called only when
        # printing (-p), where variables are expanded and the buffer overflows.
        tools = Tools(session=session)
        tools.db_connect(driver="sqlite", database=overlong_template)
        with pytest.raises(CalledModuleError, match=r"too long|exceeds.*characters"):
            tools.db_connect(flags="p")


def test_db_connect_variable_expansion_normal(session):
    """Test that normal variable expansion works correctly."""
    template = "$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/test.db"

    tools = Tools(session=session)
    tools.db_connect(driver="sqlite", database=template)
    output = tools.db_connect(flags="p", format="json")
    assert output["database_template"] == template
    assert "$GISDBASE" not in output["database"]
    assert "$LOCATION_NAME" not in output["database"]
    assert "$MAPSET" not in output["database"]
