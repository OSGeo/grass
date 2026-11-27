"""Tests of v.db.addtable"""

from pathlib import Path

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.mark.parametrize(
    ("database", "created"),
    [
        ("$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/$MAP.db", "sqlite/points.db"),
        (
            "$GISDBASE/$LOCATION_NAME/$MAPSET/vector/$MAP/sqlite.db",
            "vector/points/sqlite.db",
        ),
    ],
)
def test_map_variable_in_database_path(xy_empty_dataset_session, database, created):
    """$MAP in the database path is replaced by the map name"""
    session = xy_empty_dataset_session
    tools = Tools(session=session)
    tools.db_connect(driver="sqlite", database=database)
    tools.v_random(output="points", npoints=5, seed=1)
    tools.v_db_addtable(map="points", columns="value integer")
    assert "value" in tools.v_info(map="points", flags="c").text
    env = gs.gisenv(env=session.env)
    mapset = Path(env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"])
    assert (mapset / created).is_file()
