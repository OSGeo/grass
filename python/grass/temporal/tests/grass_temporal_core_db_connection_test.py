# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for closing temporal database connections in grass.temporal.core"""

import os
import sqlite3

import pytest

import grass.script as gs
import grass.temporal as tgis


@pytest.fixture
def temporal_session(tmp_path, monkeypatch):
    """Session in an XY project with an initialized temporal database"""
    project = tmp_path / "project"
    gs.create_project(project)
    # tgis reads the session from os.environ, so mirror the session there
    # via monkeypatch, which restores the environment after the test.
    with gs.setup.init(project, env=os.environ.copy()) as session:
        for key, value in session.env.items():
            if os.environ.get(key) != value:
                monkeypatch.setenv(key, value)
        tgis.init()
        yield


def connect_to_temporal_database():
    """Return a connected DBConnection to the database of the current mapset"""
    db = tgis.DBConnection(backend="sqlite", dbstring=tgis.get_tgis_database_string())
    db.connect()
    return db


@pytest.mark.usefixtures("temporal_session")
def test_close_closes_the_sqlite_connection():
    """After close(), the SQLite connection cannot be used anymore"""
    db = connect_to_temporal_database()
    sqlite_connection = db.connection
    db.close()
    with pytest.raises(sqlite3.ProgrammingError):
        sqlite_connection.execute("SELECT 1")


@pytest.mark.usefixtures("temporal_session")
def test_second_close_does_nothing():
    """A second close() does not fail, tgis.init() relies on that"""
    db = connect_to_temporal_database()
    db.close()
    db.close()
    assert not db.is_connected()
