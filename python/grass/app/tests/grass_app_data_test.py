"""Tests for startup database discovery."""

import os
import sys
from pathlib import Path

import pytest

from grass.app import data


@pytest.fixture
def home(tmp_path, monkeypatch):
    """Keep each test out of the real home directory."""
    home_dir = tmp_path / "home"
    home_dir.mkdir()
    monkeypatch.setenv("HOME", str(home_dir))
    monkeypatch.setenv("USERPROFILE", str(home_dir))
    return home_dir


def deny_listing(monkeypatch, directory, error_type=PermissionError):
    """Simulate a directory that cannot be listed."""
    original_scandir = os.scandir
    original_listdir = os.listdir

    def scandir(path):
        if Path(path) == directory:
            raise error_type(path)
        return original_scandir(path)

    def listdir(path):
        if Path(path) == directory:
            raise error_type(path)
        return original_listdir(path)

    # Path.iterdir uses listdir or scandir depending on the Python version.
    monkeypatch.setattr(os, "scandir", scandir)
    monkeypatch.setattr(os, "listdir", listdir)


@pytest.mark.parametrize("parent", [".", "Documents"])
@pytest.mark.parametrize("name", ["grassdata", "GrassData", "my_grassdata_backup"])
def test_find_database(home, parent, name):
    """Match grassdata names in either search directory."""
    database = home / parent / name
    database.mkdir(parents=True)

    assert data.get_possible_database_path() == str(database)


def test_home_precedes_documents(home):
    """Search home before Documents."""
    database = home / "grassdata"
    database.mkdir()
    (home / "Documents" / "grassdata").mkdir(parents=True)

    assert data.get_possible_database_path() == str(database)


@pytest.mark.parametrize("parent", [".", "Documents"])
def test_ignore_matching_file(home, parent):
    """Ignore regular files named grassdata."""
    directory = home / parent
    directory.mkdir(exist_ok=True)
    (directory / "grassdata").touch()

    assert data.get_possible_database_path() is None


@pytest.mark.parametrize("parent", [".", "Documents"])
def test_do_not_search_recursively(home, parent):
    """Only search immediate children."""
    (home / parent / "other" / "grassdata").mkdir(parents=True)

    assert data.get_possible_database_path() is None


def test_missing_documents(home):
    """Skip a missing Documents directory."""
    assert data.get_possible_database_path() is None


def test_documents_is_file(home):
    """Skip Documents when it is a file."""
    (home / "Documents").touch()

    assert data.get_possible_database_path() is None


@pytest.mark.parametrize("parent", [".", "Documents"])
@pytest.mark.parametrize("error_type", [PermissionError, FileNotFoundError, OSError])
def test_skip_unreadable_candidate(home, monkeypatch, parent, error_type):
    """Skip a directory when listing it fails."""
    candidate = home / parent
    candidate.mkdir(exist_ok=True)
    deny_listing(monkeypatch, candidate, error_type)

    assert data.get_possible_database_path() is None


def test_search_documents_after_unreadable_home(home, monkeypatch):
    """Still try Documents when home cannot be listed."""
    database = home / "Documents" / "grassdata"
    database.mkdir(parents=True)
    deny_listing(monkeypatch, home)

    assert data.get_possible_database_path() == str(database)


def test_existing_database_precedes_unreadable_documents(home, monkeypatch):
    """Use the existing database even when Documents is blocked."""
    database = home / "grassdata"
    database.mkdir()
    documents = home / "Documents"
    documents.mkdir()
    deny_listing(monkeypatch, documents)

    assert data.get_possible_database_path() == str(database)


def test_skip_unreadable_child(home, monkeypatch):
    """Keep searching after an inaccessible entry."""
    blocked = home / "grassdata_unreadable"
    blocked.mkdir()
    database = home / "grassdata"
    database.mkdir()
    original_iterdir = Path.iterdir
    original_is_dir = Path.is_dir

    def iterdir(path):
        if path == home:
            return iter([blocked, database])
        return original_iterdir(path)

    def is_dir(path):
        if path == blocked:
            raise PermissionError(path)
        return original_is_dir(path)

    monkeypatch.setattr(Path, "iterdir", iterdir)
    monkeypatch.setattr(Path, "is_dir", is_dir)

    assert data.get_possible_database_path() == str(database)


def test_error_while_advancing_iterator(home, monkeypatch):
    """Handle listing errors raised by a lazy iterator."""
    database = home / "Documents" / "grassdata"
    database.mkdir(parents=True)
    original_iterdir = Path.iterdir

    def iterdir(path):
        if path == home:
            raise PermissionError(path)
        yield from original_iterdir(path)

    monkeypatch.setattr(Path, "iterdir", iterdir)

    assert data.get_possible_database_path() == str(database)


@pytest.mark.parametrize("target_type", ["directory", "file", "missing"])
def test_database_symlink(home, tmp_path, target_type):
    """Accept directory symlinks, not file or broken links."""
    target = tmp_path / "target"
    if target_type == "directory":
        target.mkdir()
    elif target_type == "file":
        target.touch()
    link = home / "grassdata"
    try:
        link.symlink_to(target, target_is_directory=target_type == "directory")
    except (NotImplementedError, OSError):
        pytest.skip("Creating symbolic links is not supported")

    expected = str(link) if target_type == "directory" else None
    assert data.get_possible_database_path() == expected


@pytest.mark.skipif(sys.platform == "win32", reason="Unix home-directory fallback")
def test_fallback_after_unreadable_documents(home, monkeypatch):
    """Create the usual fallback when Documents cannot be read."""
    documents = home / "Documents"
    documents.mkdir()
    deny_listing(monkeypatch, documents)

    assert data.get_possible_database_path() is None
    database = data.create_database_directory()
    assert database == str(home / "grassdata")
    assert Path(database).is_dir()
    assert data.get_possible_database_path() == database
