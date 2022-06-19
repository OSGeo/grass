"""Test functions in grass.script.setup"""

import multiprocessing
import os

import pytest

import grass.script as gs


# All init tests change the global environment, but when it really matters,
# we use a separate process.
# Ideally, the functions would support env parameter and the test
# would mostly use that.
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
    with gs.setup.init(tmp_path / location):
        gs.run_command("g.region", flags="p")
        session_file = os.environ["GISRC"]
    assert not os.path.exists(session_file)


def test_init_session_finish(tmp_path):
    """Check that init works with finish on the returned session object"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    session = gs.setup.init(tmp_path / location)
    gs.run_command("g.region", flags="p")
    session_file = os.environ["GISRC"]
    session.finish()
    with pytest.raises(ValueError):
        session.finish()
    assert not session.active
    assert not os.path.exists(session_file)


def test_init_finish_global_functions(tmp_path):
    """Check that init and finish global functions work"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    gs.setup.init(tmp_path / location)
    gs.run_command("g.region", flags="p")
    session_file = os.environ["GISRC"]
    gs.setup.finish()

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
