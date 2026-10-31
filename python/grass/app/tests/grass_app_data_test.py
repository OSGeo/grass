import sys
from pathlib import Path

import pytest

from grass.app import data


@pytest.fixture
def home(tmp_path, monkeypatch):
    home_dir = tmp_path / "home"
    home_dir.mkdir()
    monkeypatch.setenv("HOME", str(home_dir))
    monkeypatch.setenv("USERPROFILE", str(home_dir))
    return home_dir


def deny_listing(monkeypatch, directory):
    original_iterdir = Path.iterdir

    def iterdir(path):
        if path == directory:
            raise PermissionError(path)
        return original_iterdir(path)

    monkeypatch.setattr(Path, "iterdir", iterdir)


@pytest.mark.parametrize("parent", [".", "Documents"])
@pytest.mark.parametrize("name", ["grassdata", "GrassData", "my_grassdata_backup"])
def test_find_database(home, parent, name):
    database = home / parent / name
    database.mkdir(parents=True)

    assert data.get_possible_database_path() == str(database)


def test_home_precedes_documents(home):
    database = home / "grassdata"
    database.mkdir()
    (home / "Documents" / "grassdata").mkdir(parents=True)

    assert data.get_possible_database_path() == str(database)


@pytest.mark.parametrize("parent", [".", "Documents"])
def test_ignore_matching_file(home, parent):
    directory = home / parent
    directory.mkdir(exist_ok=True)
    (directory / "grassdata").touch()

    assert data.get_possible_database_path() is None


@pytest.mark.parametrize("parent", [".", "Documents"])
def test_do_not_search_recursively(home, parent):
    (home / parent / "other" / "grassdata").mkdir(parents=True)

    assert data.get_possible_database_path() is None


def test_missing_documents(home):
    assert data.get_possible_database_path() is None


def test_documents_is_file(home):
    (home / "Documents").touch()

    assert data.get_possible_database_path() is None


@pytest.mark.parametrize("parent", [".", "Documents"])
def test_skip_unreadable_candidate(home, monkeypatch, parent):
    candidate = home / parent
    candidate.mkdir(exist_ok=True)
    deny_listing(monkeypatch, candidate)

    assert data.get_possible_database_path() is None


def test_search_documents_after_unreadable_home(home, monkeypatch):
    database = home / "Documents" / "grassdata"
    database.mkdir(parents=True)
    deny_listing(monkeypatch, home)

    assert data.get_possible_database_path() == str(database)


def test_existing_database_precedes_unreadable_documents(home, monkeypatch):
    database = home / "grassdata"
    database.mkdir()
    documents = home / "Documents"
    documents.mkdir()
    deny_listing(monkeypatch, documents)

    assert data.get_possible_database_path() == str(database)


def test_skip_unreadable_child(home, monkeypatch):
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
    documents = home / "Documents"
    documents.mkdir()
    deny_listing(monkeypatch, documents)

    assert data.get_possible_database_path() is None
    database = data.create_database_directory()
    assert database == str(home / "grassdata")
    assert Path(database).is_dir()
    assert data.get_possible_database_path() == database
