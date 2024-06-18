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


def test_init_finish_global_functions_with_env(tmp_path):
    """Check that init and finish global functions work"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    env = os.environ.copy()
    gs.setup.init(tmp_path / location, env=env)
    gs.run_command("g.region", flags="p", env=env)
    session_file = env["GISRC"]
    gs.setup.finish(env=env)

    assert not os.path.exists(session_file)


def test_init_finish_global_functions_capture_strerr0(tmp_path):
    """Check that init and finish global functions work with global env"""

    def init_finish(queue):
        gs.set_capture_stderr(True)
        location = "test"
        gs.core._create_location_xy(  # pylint: disable=protected-access
            tmp_path, location
        )
        gs.setup.init(tmp_path / location)
        gs.run_command("g.region", flags="p")
        runtime_present = bool(os.environ.get("GISBASE"))
        queue.put((os.environ["GISRC"], runtime_present))
        gs.setup.finish()

    session_file, runtime_present = run_in_subprocess(init_finish)
    assert session_file, "Expected file name from the subprocess"
    assert runtime_present, "Runtime (GISBASE) should be present"
    assert not os.path.exists(session_file), "Session file not deleted"


def test_init_finish_global_functions_capture_strerrX(tmp_path):
    """Check that init and finish global functions work with global env"""

    def init_finish(queue):
        gs.set_capture_stderr(True)
        location = "test"
        gs.core._create_location_xy(  # pylint: disable=protected-access
            tmp_path, location
        )
        gs.setup.init(tmp_path / location)
        gs.run_command("g.region", flags="p")
        runtime_present = bool(os.environ.get("GISBASE"))
        session_file = os.environ["GISRC"]
        gs.setup.finish()
        runtime_present_after = bool(os.environ.get("GISBASE"))
        queue.put((session_file, runtime_present, runtime_present_after))

    session_file, runtime_present, runtime_present_after = run_in_subprocess(
        init_finish
    )
    assert session_file, "Expected file name from the subprocess"
    assert runtime_present, "Runtime (GISBASE) should be present"
    assert not os.path.exists(session_file), "Session file not deleted"
    # This is testing the current implementation behavior, but it is not required
    # to be this way in terms of design.
    assert runtime_present_after, "Runtime should continue to be present"


def test_init_finish_global_functions_isolated(tmp_path):
    """Check that init and finish global functions work with global env"""

    def init_finish(queue):
        gs.set_capture_stderr(True)
        location = "test"
        gs.core._create_location_xy(  # pylint: disable=protected-access
            tmp_path, location
        )
        gs.setup.init(tmp_path / location)
        gs.run_command("g.region", flags="p")
        runtime_present_during = bool(os.environ.get("GISBASE"))
        session_file_variable_present_during = bool(os.environ.get("GISRC"))
        session_file = os.environ.get("GISRC")
        if session_file:
            session_file_present_during = os.path.exists(session_file)
        else:
            session_file_present_during = False
        gs.setup.finish()
        session_file_variable_present_after = bool(os.environ.get("GISRC"))
        runtime_present_after = bool(os.environ.get("GISBASE"))
        queue.put(
            (
                session_file,
                session_file_variable_present_during,
                session_file_present_during,
                session_file_variable_present_after,
                runtime_present_during,
                runtime_present_after,
            )
        )

    (
        session_file,
        session_file_variable_present_during,
        session_file_present_during,
        session_file_variable_present_after,
        runtime_present_during,
        runtime_present_after,
    ) = run_in_subprocess(init_finish)

    # Runtime
    assert runtime_present_during, "Runtime (GISBASE) should be present"
    # This is testing the current implementation behavior, but it is not required
    # to be this way in terms of design.
    assert runtime_present_after, "Expected GISBASE to be present when finished"

    # Session
    assert session_file_present_during, "Expected session file to be present"
    assert session_file_variable_present_during, "Variable GISRC should be present"
    assert not session_file_variable_present_after, "Not expecting GISRC when finished"
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
            runtime_present = bool(os.environ.get("GISBASE"))
            queue.put((session_file, os.path.exists(session_file), runtime_present))

    session_file, file_existed, runtime_present = run_in_subprocess(workload)
    assert session_file, "Expected file name from the subprocess"
    assert file_existed, "File should have been present"
    assert runtime_present, "Runtime (GISBASE) should be present"
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
