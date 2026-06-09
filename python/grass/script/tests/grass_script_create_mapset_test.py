"""Tests for grass.script.create_mapset"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def project_path(tmp_path):
    """Create a XY project and return its path."""
    project = tmp_path / "test_project"
    gs.create_project(project)
    return project


def test_create_mapset_full_path(project_path):
    """Check that mapset is created when full path is given."""
    mapset_path = project_path / "new_mapset"
    gs.create_mapset(mapset_path, initialize_db=False)
    assert mapset_path.exists()
    assert (mapset_path / "WIND").exists()


def test_create_mapset_string_path(project_path):
    """Check that mapset is created when path is given as a string."""
    gs.create_mapset(str(project_path), name="new_mapset", initialize_db=False)
    mapset_path = project_path / "new_mapset"
    assert mapset_path.exists()
    assert (mapset_path / "WIND").exists()


def test_create_mapset_with_name(project_path):
    """Check that mapset is created when project path and name are given."""
    gs.create_mapset(project_path, name="new_mapset", initialize_db=False)
    mapset_path = project_path / "new_mapset"
    assert mapset_path.exists()
    assert (mapset_path / "WIND").exists()


def test_create_mapset_usable(project_path):
    """Check that a created mapset can be used in a session."""
    gs.create_mapset(project_path, name="new_mapset")
    with gs.setup.init(project_path / "new_mapset", env=os.environ.copy()) as session:
        info = gs.gisenv(env=session.env)
        assert info["MAPSET"] == "new_mapset"


def test_create_mapset_no_overwrite(project_path):
    """Check that existing mapset raises error without overwrite."""
    gs.create_mapset(project_path, name="new_mapset")
    with pytest.raises(ValueError, match="already exists"):
        gs.create_mapset(project_path, name="new_mapset")


def test_create_mapset_overwrite(project_path):
    """Check that existing mapset can be overwritten."""
    mapset_path = project_path / "new_mapset"
    gs.create_mapset(project_path, name="new_mapset", initialize_db=False)
    # Add a file to verify old content is removed
    marker = mapset_path / "marker_file"
    marker.write_text("test")
    assert marker.exists()

    gs.create_mapset(
        project_path, name="new_mapset", overwrite=True, initialize_db=False
    )
    assert mapset_path.exists()
    assert (mapset_path / "WIND").exists()
    assert not marker.exists()


def test_create_mapset_nonexistent_project(tmp_path):
    """Check error when project does not exist."""
    with pytest.raises(ValueError, match="does not exist"):
        gs.create_mapset(tmp_path / "nonexistent" / "new_mapset")


def test_create_mapset_invalid_project(tmp_path):
    """Check error when project directory exists but has no PERMANENT mapset."""
    invalid_project = tmp_path / "invalid_project"
    invalid_project.mkdir()
    with pytest.raises(ValueError, match="PERMANENT"):
        gs.create_mapset(invalid_project, name="new_mapset")


def test_create_mapset_permanent_rejected(project_path):
    """Check that creating PERMANENT mapset is rejected."""
    with pytest.raises(ValueError, match="PERMANENT"):
        gs.create_mapset(project_path, name="PERMANENT")


@pytest.mark.parametrize("name", [".hidden", "has space", "with@at"])
def test_create_mapset_illegal_name(project_path, name):
    """Check that illegal mapset names are rejected."""
    with pytest.raises(ValueError, match="Illegal"):
        gs.create_mapset(project_path, name=name, initialize_db=False)


def test_create_multiple_mapsets(project_path):
    """Check that multiple mapsets can be created in the same project."""
    names = ["mapset_a", "mapset_b", "mapset_c"]
    for name in names:
        gs.create_mapset(project_path, name=name, initialize_db=False)
    for name in names:
        assert (project_path / name).exists()
        assert (project_path / name / "WIND").exists()


def test_create_mapset_name_only(project_path):
    """Check that mapset is created using only name within an active session."""
    with gs.setup.init(project_path, env=os.environ.copy()) as session:
        gs.create_mapset(name="new_mapset", env=session.env)
    mapset_path = project_path / "new_mapset"
    assert mapset_path.exists()
    assert (mapset_path / "WIND").exists()


@pytest.mark.usefixtures("mock_no_session")
def test_create_mapset_name_only_no_session():
    """Check that name-only fails without an active session."""
    with pytest.raises(ValueError, match="No active session"):
        gs.create_mapset(name="new_mapset")


def test_create_mapset_no_arguments():
    """Check that calling without path or name raises an error."""
    with pytest.raises(ValueError, match="Either path or name"):
        gs.create_mapset()


def test_create_mapset_db_initialized(project_path):
    """Check that database connection is initialized by default."""
    gs.create_mapset(project_path, name="new_mapset")
    mapset_path = project_path / "new_mapset"
    assert (mapset_path / "VAR").exists()
    with gs.setup.init(mapset_path, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        conn = tools.db_connect(flags="p", format="json")
        assert conn["driver"] == "sqlite"
        assert "new_mapset" in conn["database"]


def test_create_mapset_db_not_initialized(project_path):
    """Check that database initialization is skipped when disabled."""
    gs.create_mapset(project_path, name="new_mapset", initialize_db=False)
    mapset_path = project_path / "new_mapset"
    assert mapset_path.exists()
    assert (mapset_path / "WIND").exists()
    assert not (mapset_path / "VAR").exists()


def test_create_mapset_with_existing_session_env(project_path):
    """Check that creating a mapset works when env comes from an existing session.

    The env already has a GISRC pointing to PERMANENT. The DB initialization
    must create its own session for the new mapset, not reuse the old one.
    """
    with gs.setup.init(project_path, env=os.environ.copy()) as session:
        gs.create_mapset(project_path, name="new_mapset", env=session.env)
    mapset_path = project_path / "new_mapset"
    assert mapset_path.exists()
    assert (mapset_path / "WIND").exists()
    assert (mapset_path / "VAR").exists()
    with gs.setup.init(mapset_path, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        conn = tools.db_connect(flags="p", format="json")
        assert conn["driver"] == "sqlite"
        assert "new_mapset" in conn["database"]


def test_create_mapset_region_from_default(project_path):
    """Check that the new mapset's region matches the project's default region."""
    with gs.setup.init(project_path, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=100, s=0, e=200, w=0, res=10, flags="s")
    gs.create_mapset(project_path, name="new_mapset")
    with gs.setup.init(project_path / "new_mapset", env=os.environ.copy()) as session:
        tools = Tools(session=session)
        region = tools.g_region(flags="p", format="json")
        assert region["north"] == 100
        assert region["south"] == 0
        assert region["east"] == 200
        assert region["west"] == 0
