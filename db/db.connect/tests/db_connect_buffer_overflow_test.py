import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError


def test_db_connect_normal_database_name(session):
    """Test that db.connect works with normal database names."""
    gs.run_command("db.connect", flags="d", env=session.env)

    output = gs.parse_command("db.connect", flags="p", format="json", env=session.env)
    assert output["driver"] == "sqlite"
    assert "sqlite.db" in output["database"]


def test_db_connect_long_valid_database_name(session):
    """Test that db.connect works with long but valid database names."""
    long_name = "/tmp/" + "a" * 2000 + "/test.db"

    try:
        gs.run_command("db.connect", driver="sqlite", database=long_name, env=session.env)
        output = gs.read_command("db.connect", flags="p", env=session.env)
        assert "test.db" in output
    except CalledModuleError as e:
        error_msg = str(e).lower()
        assert "too long" not in error_msg
        assert "exceeds" not in error_msg


def test_db_connect_overlong_database_name_rejected(session):
    """Test that db.connect rejects database names exceeding GPATH_MAX."""
    overlong_name = "/tmp/" + "x" * 4500 + "/test.db"

    with pytest.raises(CalledModuleError, match=r"too long|exceeds.*characters"):
        gs.run_command("db.connect", driver="sqlite", database=overlong_name, env=session.env)


def test_db_connect_variable_expansion_overflow(session):
    """Test that variable expansion is bounds-checked."""
    gisenv = gs.gisenv(env=session.env)
    gisdbase = gisenv["GISDBASE"]
    location = gisenv["LOCATION_NAME"]
    mapset = gisenv["MAPSET"]

    expanded_length = len(gisdbase) + len(location) + len(mapset) + 100
    padding_needed = 4096 - expanded_length + 500

    if padding_needed > 0:
        overlong_template = "$GISDBASE/" + "x" * padding_needed + "/$LOCATION_NAME/$MAPSET/test.db"

        with pytest.raises(CalledModuleError, match=r"too long|exceeds.*characters"):
            gs.run_command("db.connect", driver="sqlite", database=overlong_template, env=session.env)


def test_db_connect_variable_expansion_normal(session):
    """Test that normal variable expansion works correctly."""
    template = "$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/test.db"

    gs.run_command("db.connect", driver="sqlite", database=template, env=session.env)

    output = gs.parse_command("db.connect", flags="p", format="json", env=session.env)
    assert output["database_template"] == template
    assert "$GISDBASE" not in output["database"]
    assert "$LOCATION_NAME" not in output["database"]
    assert "$MAPSET" not in output["database"]
