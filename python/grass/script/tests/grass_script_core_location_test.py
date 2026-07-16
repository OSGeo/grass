"""Test functions in grass.script.setup"""

import os

import pytest

import grass.script as gs
from grass.exceptions import ScriptError
from grass.tools import Tools


def test_with_same_path(tmp_path):
    """Check correct EPSG is created with same path as the current one.

    Creates two projects, a XY bootstrap one and a desired project using
    the same path (build for the case when only XY project can be created
    which is no longer the case, but still it makes sense to test that as
    a possible situation).
    """
    bootstrap = "bootstrap"
    desired = "desired"
    gs.create_project(tmp_path / bootstrap)
    with gs.setup.init(tmp_path / bootstrap, env=os.environ.copy()) as session:
        gs.create_location(tmp_path / desired, epsg="3358")
        assert (tmp_path / desired).exists()
        wkt_file = tmp_path / desired / "PERMANENT" / "PROJ_WKT"
        assert wkt_file.exists()
        gs.run_command("g.gisenv", set=f"GISDBASE={tmp_path}", env=session.env)
        gs.run_command("g.gisenv", set=f"LOCATION_NAME={desired}", env=session.env)
        gs.run_command("g.gisenv", set="MAPSET=PERMANENT", env=session.env)
        srid = gs.parse_command("g.proj", flags="p", format="shell", env=session.env)[
            "srid"
        ]
    assert srid == "EPSG:3358"


@pytest.mark.usefixtures("mock_no_session")
def test_without_session(tmp_path):
    """Check that creation works outside of session.

    Assumes that there is no session for the test. This can be ensured by running only
    this test with pytest outside of a session.

    Also checks that the global environment is intact after calling the function.
    """

    name = "desired"
    gs.create_location(tmp_path, name, epsg="3358")

    # Check that the global environment is still intact.
    assert not os.environ.get("GISRC"), "Session exists after the call"
    assert not os.environ.get("GISBASE"), "Runtime exists after the call"

    assert (tmp_path / name).exists()
    wkt_file = tmp_path / name / "PERMANENT" / "PROJ_WKT"
    assert wkt_file.exists()
    with gs.setup.init(tmp_path / name, env=os.environ.copy()) as session:
        epsg = gs.parse_command("g.proj", flags="p", format="shell", env=session.env)[
            "srid"
        ]
        assert epsg == "EPSG:3358"


def test_with_different_path(tmp_path):
    """Check correct EPSG is created with different path"""
    bootstrap = "bootstrap"
    desired = "desired"
    tmp_path_a = tmp_path / "a"
    tmp_path_b = tmp_path / "b"
    tmp_path_a.mkdir()
    gs.create_project(tmp_path_a / bootstrap)
    with gs.setup.init(tmp_path_a / bootstrap, env=os.environ.copy()) as session:
        gs.create_location(tmp_path_b, desired, epsg="3358")
        assert (tmp_path_b / desired).exists()
        wkt_file = tmp_path_b / desired / "PERMANENT" / "PROJ_WKT"
        assert wkt_file.exists()
        gs.run_command("g.gisenv", set=f"GISDBASE={tmp_path_b}", env=session.env)
        gs.run_command("g.gisenv", set=f"LOCATION_NAME={desired}", env=session.env)
        gs.run_command("g.gisenv", set="MAPSET=PERMANENT", env=session.env)
        epsg = gs.parse_command("g.proj", flags="p", format="shell", env=session.env)[
            "srid"
        ]
        assert epsg == "EPSG:3358"


def test_path_only(tmp_path):
    full_path = tmp_path / "desired"
    gs.create_location(full_path, epsg="3358")
    mapset_path = full_path / "PERMANENT"
    wkt_file = mapset_path / "PROJ_WKT"
    assert full_path.exists()
    assert mapset_path.exists()
    assert wkt_file.exists()
    with gs.setup.init(full_path, env=os.environ.copy()) as session:
        epsg = gs.parse_command("g.proj", flags="p", format="shell", env=session.env)[
            "srid"
        ]
        assert epsg == "EPSG:3358"


def test_create_project(tmp_path):
    name = "desired"
    gs.create_project(tmp_path / name, epsg="3358")
    assert (tmp_path / name).exists()
    wkt_file = tmp_path / name / "PERMANENT" / "PROJ_WKT"
    assert wkt_file.exists()
    with gs.setup.init(tmp_path / name, env=os.environ.copy()) as session:
        epsg = gs.parse_command("g.proj", flags="p", format="shell", env=session.env)[
            "srid"
        ]
        assert epsg == "EPSG:3358"


@pytest.mark.parametrize("crs", ["XY", "xy", None, ""])
def test_crs_parameter_xy(tmp_path, crs):
    project = tmp_path / "test"
    gs.create_project(project, crs=crs)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="shell").keyval["projection"] == 0


def test_crs_parameter_xy_overrides_epsg(tmp_path):
    project = tmp_path / "test"
    gs.create_project(project, crs="XY", epsg=4326)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["crs"]["type"] == "xy"


def test_crs_parameter_epsg_overrides_epsg(tmp_path):
    project = tmp_path / "test"
    gs.create_project(project, crs="EPSG:4326", epsg=3358)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_proj(flags="p", format="shell").keyval["srid"] == "EPSG:4326"


@pytest.mark.parametrize("crs", ["EPSG:4326", "epsg:3358"])
def test_crs_parameter_epsg(tmp_path, crs):
    project = tmp_path / "test"
    gs.create_project(project, crs=crs)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_proj(flags="p", format="shell").keyval["srid"] == crs.upper()


def test_files(tmp_path):
    """Check expected files are created with bootstrap and session"""
    bootstrap = "bootstrap"
    desired = "desired"
    gs.create_project(tmp_path / bootstrap)
    with gs.setup.init(tmp_path / bootstrap, env=os.environ.copy()):
        description = "This is a test (not Gauss-Krüger or Křovák)"
        gs.create_project(tmp_path, desired, epsg="3358", desc=description)
        assert (tmp_path / desired).exists()
        base_path = tmp_path / desired / "PERMANENT"
        assert (base_path / "PROJ_WKT").exists()
        assert (base_path / "PROJ_SRID").exists()
        assert (base_path / "PROJ_UNITS").exists()
        assert (base_path / "PROJ_INFO").exists()
        assert (base_path / "DEFAULT_WIND").exists()
        assert (base_path / "WIND").exists()
        description_file = base_path / "MYNAME"
        assert description_file.exists()
        assert description_file.read_text(encoding="utf-8").strip() == description


@pytest.mark.parametrize("overwrite", [False, None])
def test_project_no_overwrite(tmp_path, overwrite):
    """Check that project we raise exception when project exists"""
    project = tmp_path / "project_1"
    gs.create_project(project, overwrite=overwrite)
    assert project.exists()
    with pytest.raises(ScriptError, match=r"project_1.*already exists"):
        gs.create_project(project, overwrite=overwrite)
    assert project.exists()


def test_project_overwrite(tmp_path):
    """Check that existing project can be overwritten"""
    project = tmp_path / "project_1"
    gs.create_project(project)
    assert project.exists()
    gs.create_project(project, overwrite=True)
    assert project.exists()


def create_project_get_description_file(tmp_path, text):
    """Set text as description and check the result"""
    name = "test"
    gs.create_project(tmp_path / name, description=text)
    return tmp_path / name / "PERMANENT" / "MYNAME"


def test_location_description_setter_ascii(tmp_path):
    """Check ASCII text"""
    text = "This is a test"
    description_file = create_project_get_description_file(tmp_path, text)
    assert description_file.exists()
    assert description_file.read_text(encoding="utf-8").strip() == text


def test_location_description_setter_unicode(tmp_path):
    """Check unicode text"""
    text = "This is a test (not Gauss-Krüger or Křovák)"
    description_file = create_project_get_description_file(tmp_path, text)
    assert description_file.exists()
    assert description_file.read_text(encoding="utf-8").strip() == text


def test_location_description_setter_empty(tmp_path):
    """Check empty text"""
    description_file = create_project_get_description_file(tmp_path, "")
    # Explicitly set empty is passed through to the file.
    assert description_file.exists()
    assert description_file.read_text(encoding="utf-8").strip() == ""


def test_location_description_setter_none(tmp_path):
    """Check None in place of text"""
    description_file = create_project_get_description_file(tmp_path, None)
    assert not description_file.exists()
