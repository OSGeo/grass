# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for connections and transactions of DBConnection in grass.temporal.core"""

import os
import sqlite3
from contextlib import closing

import pytest

import grass.script as gs
import grass.temporal as tgis

FAILING_TRANSACTION = "INSERT INTO t VALUES (1); INSERT INTO no_such_table VALUES (2)"
OTHER_ERROR = "not a database error"


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


@pytest.fixture
def table_path(tmp_path):
    """Path of an SQLite database with an empty table t"""
    path = tmp_path / "transactions.db"
    with closing(sqlite3.connect(path)) as connection:
        connection.execute("CREATE TABLE t (x INTEGER)")
    return path


def connect_to_temporal_database():
    """Return a connected DBConnection to the database of the current mapset"""
    db = tgis.DBConnection(backend="sqlite", dbstring=tgis.get_tgis_database_string())
    db.connect()
    return db


def connect_to(path):
    """Return a connected DBConnection to the SQLite database at path"""
    db = tgis.DBConnection(backend="sqlite", dbstring=str(path))
    db.connect()
    return db


def saved_rows(path):
    """Return the values in table t, read with a new connection"""
    with closing(sqlite3.connect(path)) as connection:
        return [row[0] for row in connection.execute("SELECT x FROM t ORDER BY x")]


class InterruptingCursor:
    """Cursor which starts the transaction, then fails with a non-database error"""

    def __init__(self, cursor):
        self.cursor = cursor

    def executescript(self, script):
        self.cursor.executescript("BEGIN TRANSACTION; INSERT INTO t VALUES (1);")
        raise RuntimeError(OTHER_ERROR)


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


@pytest.mark.usefixtures("temporal_session")
def test_close_rolls_back_an_open_transaction(table_path):
    """close() does not commit a transaction that is still open"""
    db = connect_to(table_path)
    db.execute("BEGIN TRANSACTION")
    db.execute("INSERT INTO t VALUES (1)")
    db.close()
    assert saved_rows(table_path) == []


@pytest.mark.usefixtures("temporal_session")
def test_successful_transaction_is_saved(table_path):
    """A transaction without errors is saved despite the rollback() in finally"""
    db = tgis.DBConnection(backend="sqlite", dbstring=str(table_path))
    db.execute_transaction("INSERT INTO t VALUES (1); INSERT INTO t VALUES (2)")
    assert saved_rows(table_path) == [1, 2]


@pytest.mark.usefixtures("temporal_session")
def test_failed_transaction_saves_nothing(table_path):
    """No statement of a failed transaction is saved"""
    db = tgis.DBConnection(backend="sqlite", dbstring=str(table_path))
    with pytest.raises(sqlite3.OperationalError, match="no such table"):
        db.execute_transaction(FAILING_TRANSACTION)
    assert saved_rows(table_path) == []


@pytest.mark.usefixtures("temporal_session")
def test_failed_transaction_is_not_saved_by_the_next_one(table_path):
    """A failed transaction leaves nothing open on the caller's connection"""
    db = connect_to(table_path)
    with pytest.raises(sqlite3.OperationalError, match="no such table"):
        db.execute_transaction(FAILING_TRANSACTION)
    assert not db.connection.in_transaction
    db.execute_transaction("INSERT INTO t VALUES (3)")
    db.close()
    assert saved_rows(table_path) == [3]


@pytest.mark.usefixtures("temporal_session")
def test_transaction_stopped_by_other_error_is_not_saved(table_path):
    """A transaction which a non-database error stopped is not saved later"""
    db = connect_to(table_path)
    cursor = db.cursor
    db.cursor = InterruptingCursor(cursor)
    with pytest.raises(RuntimeError, match=OTHER_ERROR):
        db.execute_transaction("INSERT INTO t VALUES (1)")
    db.cursor = cursor
    db.execute_transaction("INSERT INTO t VALUES (3)")
    db.close()
    assert saved_rows(table_path) == [3]
