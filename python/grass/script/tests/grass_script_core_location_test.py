"""Test functions in grass.script.setup"""

import multiprocessing
import os

import pytest

import grass.script as gs

xfail_mp_spawn = pytest.mark.xfail(
    multiprocessing.get_start_method() == "spawn",
    reason="Multiprocessing using 'spawn' start method requires pickable functions",
    raises=AttributeError,
    strict=True,
)


# This is useful when we want to ensure that function like init does
# not change the global environment.
def run_in_subprocess(function):
    """Run function in a separate process

    The function must take a Queue and put its result there.
    The result is then returned from this function.
    """
    queue = multiprocessing.Queue()
    process = multiprocessing.Process(target=function, args=(queue,))
    process.start()
    result = queue.get()
    process.join()
    return result


def create_and_get_srid(tmp_path):
    """Create location on the same path as the current one"""
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
        return gs.parse_command("g.proj", flags="p", format="shell", env=session.env)[
            "srid"
        ]


def test_with_same_path(tmp_path):
    """Check correct EPSG is created with same path as the current one"""
    srid = create_and_get_srid(tmp_path)
    assert srid == "EPSG:3358"


@xfail_mp_spawn
def test_with_init_in_subprocess(tmp_path):
    """Check creation when running in a subprocess"""

    def workload(queue):
        """Transfer the return value using queue"""
        queue.put(create_and_get_srid(tmp_path))

    epsg = run_in_subprocess(workload)
    assert epsg == "EPSG:3358"


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


def test_project_overwrite(tmp_path):
    """Check that project can be overwritten"""
    project = tmp_path / "project"
    gs.create_project(project)
    assert project.exists()
    gs.create_project(project, overwrite=True)
    assert project.exists()


def set_and_test_description(tmp_path, text):
    """Set text as description and check the result"""
    name = "test"
    gs.create_project(tmp_path / name, desc=text)
    description_file = tmp_path / name / "PERMANENT" / "MYNAME"
    # The behavior inherited in the code is that the file always needs to exist,
    # regardless of user setting the title or not. This may change in the future,
    # but for now, we test for this specific behavior.
    assert description_file.exists()
    text = text or ""  # None and empty should both yield empty.
    assert description_file.read_text(encoding="utf-8").strip() == text


def test_location_description_setter_ascii(tmp_path):
    """Check ASCII text"""
    set_and_test_description(tmp_path, "This is a test")


def test_location_description_setter_unicode(tmp_path):
    """Check unicode text"""
    set_and_test_description(tmp_path, "This is a test (not Gauss-Krüger or Křovák)")


def test_location_description_setter_empty(tmp_path):
    """Check empty text"""
    set_and_test_description(tmp_path, "")


def test_location_description_setter_none(tmp_path):
    """Check None in place of text"""
    set_and_test_description(tmp_path, None)
