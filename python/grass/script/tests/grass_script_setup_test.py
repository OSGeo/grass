"""Test functions in grass.script.setup"""

import multiprocessing
import os

import pytest

import grass.script as gs


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


def test_init_as_context_manager(tmp_path):
    """Check that init function return value works as a context manager"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        gs.run_command("g.region", flags="p", env=session.env)
        session_file = session.env["GISRC"]
        assert os.path.exists(session_file)
    assert not os.path.exists(session_file)


def test_init_session_finish(tmp_path):
    """Check that init works with finish on the returned session object"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    session = gs.setup.init(tmp_path / location, env=os.environ.copy())
    gs.run_command("g.region", flags="p", env=session.env)
    session_file = session.env["GISRC"]
    session.finish()
    with pytest.raises(ValueError):
        session.finish()
    assert not session.active
    assert not os.path.exists(session_file)


def test_init_finish_global_functions(tmp_path):
    """Check that init and finish global functions work"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    env = os.environ.copy()
    gs.setup.init(tmp_path / location, env=env)
    gs.run_command("g.region", flags="p", env=env)
    session_file = env["GISRC"]
    gs.setup.finish(env=env)

    assert not os.path.exists(session_file)


def test_init_finish_global_functions_capture_strerr(tmp_path):
    """Check that init and finish global functions work"""

    def init_finish(queue):
        gs.set_capture_stderr(True)
        location = "test"
        gs.core._create_location_xy(  # pylint: disable=protected-access
            tmp_path, location
        )
        gs.setup.init(tmp_path / location)
        gs.run_command("g.region", flags="p")
        queue.put(os.environ["GISRC"])
        gs.setup.finish()

    session_file = run_in_subprocess(init_finish)
    assert session_file, "Expected file name from the subprocess"
    assert not os.path.exists(session_file), "Session file not deleted"


@pytest.mark.usefixtures("mock_no_session")
def test_init_as_context_manager_env_attribute(tmp_path):
    """Check that session has global environment as attribute"""

    def workload(queue):
        location = "test"
        gs.core._create_location_xy(
            tmp_path, location
        )  # pylint: disable=protected-access
        with gs.setup.init(tmp_path / location) as session:
            gs.run_command("g.region", flags="p", env=session.env)
            session_file = os.environ["GISRC"]
            queue.put((session_file, os.path.exists(session_file)))

    session_file, file_existed = run_in_subprocess(workload)
    assert session_file, "Expected file name from the subprocess"
    assert file_existed, "File should have been present"
    assert not os.path.exists(session_file), "Session file not deleted"
    assert not os.environ.get("GISRC")
    assert not os.environ.get("GISBASE")


@pytest.mark.usefixtures("mock_no_session")
def test_init_environment_isolation(tmp_path):
    """Check that only the provided environment is modified"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    env = os.environ.copy()
    with gs.setup.init(tmp_path / location, env=env) as session:
        gs.run_command("g.region", flags="p", env=session.env)
        assert env.get("GISBASE")
        assert env.get("GISRC")
        # Check that the global environment is intact.
        assert not os.environ.get("GISRC")
        assert not os.environ.get("GISBASE")
    assert not env.get("GISRC")
    # We test if the global environment is intact after closing the session.
    assert not os.environ.get("GISRC")
    assert not os.environ.get("GISBASE")
