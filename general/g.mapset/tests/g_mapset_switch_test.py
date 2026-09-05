"""Tests for g.mapset switching, creation, and error handling"""

import os
import sys

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.tools import Tools


@pytest.fixture
def session(tmp_path):
    """Session with PERMANENT, test1, and test2 mapsets. Starts in test1."""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_mapset(mapset="test1", flags="c")
        tools.g_mapset(mapset="test2", flags="c")
        tools.g_mapset(mapset="test1")
        yield session


def test_switch_mapset_and_back(session):
    """Test switching to a mapset and back to the starting one.

    The fixture starts in test1, so this verifies a full round-trip.
    """
    tools = Tools(session=session)
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"
    tools.g_mapset(mapset="test2")
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test2"
    tools.g_mapset(mapset="test1")
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"


def test_switch_to_permanent(session):
    """Test switching to PERMANENT because it is special."""
    tools = Tools(session=session)
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"
    tools.g_mapset(mapset="PERMANENT")
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "PERMANENT"


def test_create_and_switch(session):
    """Test -c flag creates a new mapset and switches to it."""
    tools = Tools(session=session)
    tools.g_mapset(mapset="new_mapset", flags="c")
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "new_mapset"


def test_create_existing_mapset(session):
    """Test -c flag with an already existing mapset just switches to it."""
    tools = Tools(session=session)
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"
    tools.g_mapset(mapset="test2", flags="c")
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test2"


def test_nonexistent_mapset_keeps_current(session):
    """Test that switching to a nonexistent mapset fails and keeps current mapset."""
    tools = Tools(session=session)
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"
    with pytest.raises(CalledModuleError, match="does not exist"):
        tools.g_mapset(mapset="nonexistent")
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"


def test_list_includes_created_mapset(session):
    """Test that -l lists a newly created mapset."""
    tools = Tools(session=session)
    tools.g_mapset(mapset="listed_mapset", flags="c")
    data = tools.g_mapset(flags="l", format="json")
    assert "listed_mapset" in data["mapsets"]


def test_switch_to_current_mapset(session):
    """Test that switching to the already-current mapset succeeds with a warning."""
    tools = Tools(session=session)
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"
    tools.g_mapset(mapset="test1")
    assert tools.g_mapset(flags="p", format="json")["mapset"] == "test1"


def test_list_with_different_project(tmp_path):
    """Test -l with explicit project lists mapsets from that project."""
    project_a = tmp_path / "project_a"
    project_b = tmp_path / "project_b"
    gs.create_project(project_a)
    gs.create_project(project_b)
    with gs.setup.init(project_a, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        # Create an extra mapset in project_b
        tools.g_mapset(
            mapset="extra", dbase=str(tmp_path), project="project_b", flags="c"
        )
        # Switch back to project_a
        tools.g_mapset(mapset="PERMANENT", dbase=str(tmp_path), project="project_a")
        # List mapsets in project_b from project_a
        data = tools.g_mapset(
            flags="l", dbase=str(tmp_path), project="project_b", format="json"
        )
        assert "extra" in data["mapsets"]
        assert "PERMANENT" in data["mapsets"]


@pytest.mark.parametrize("name", [".starts_with_dot", "has space", "with/slash"])
def test_create_illegal_mapset_name(session, name):
    """Test that -c with an illegal mapset name fails."""
    tools = Tools(session=session)
    with pytest.raises(CalledModuleError, match="Unable to create"):
        tools.g_mapset(mapset=name, flags="c")


def test_list_excludes_non_mapset_directories(tmp_path):
    """Test that -l does not list plain directories that are not mapsets."""
    project = tmp_path / "test"
    gs.create_project(project)
    (project / "not_a_mapset").mkdir()
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        data = tools.g_mapset(flags="l", format="json")
        assert "not_a_mapset" not in data["mapsets"]


def test_switch_to_different_project_same_dir(tmp_path):
    """Test switching to a different project in the same directory."""
    project_a = tmp_path / "project_a"
    project_b = tmp_path / "project_b"
    gs.create_project(project_a)
    gs.create_project(project_b)
    with gs.setup.init(project_a, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_mapset(mapset="PERMANENT", project="project_b")
        data = tools.g_mapset(flags="p", format="json")
        assert data["mapset"] == "PERMANENT"
        assert data["project"] == "project_b"


def test_switch_to_project_different_dir(tmp_path):
    """Test switching to a project in a different directory."""
    dbase_a = tmp_path / "dir_a"
    dbase_b = tmp_path / "dir_b"
    dbase_a.mkdir()
    dbase_b.mkdir()
    gs.create_project(dbase_a / "project")
    gs.create_project(dbase_b / "project")
    with gs.setup.init(dbase_a / "project", env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_mapset(mapset="PERMANENT", dbase=str(dbase_b), project="project")
        data = tools.g_mapset(flags="p", format="json")
        assert data["mapset"] == "PERMANENT"
        assert data["project"] == "project"
        dbase_result = gs.read_command(
            "g.gisenv", get="GISDBASE", flags="n", env=session.env
        ).strip()
        assert dbase_result == str(dbase_b)


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="Locking is disabled on Windows"
)
def test_switch_to_locked_mapset(tmp_path):
    """Test that switching to a mapset locked by another session fails."""
    project = tmp_path / "test"
    gs.create_project(project)
    # Create both mapsets from PERMANENT
    with gs.setup.init(project, env=os.environ.copy()) as setup_session:
        tools = Tools(session=setup_session)
        tools.g_mapset(mapset="locked_one", flags="c")
        tools.g_mapset(mapset="free_one", flags="c")
    # Lock locked_one with one session, open a second session in free_one
    with (
        gs.setup.init(project / "locked_one", env=os.environ.copy(), lock=True),
        gs.setup.init(project / "free_one", env=os.environ.copy()) as session_b,
    ):
        tools_b = Tools(session=session_b)
        with pytest.raises(CalledModuleError, match="active GRASS session"):
            tools_b.g_mapset(mapset="locked_one")
