"""Test functions in grass.script.setup"""

import os
import sys

import subprocess
import json
from textwrap import dedent
from pathlib import Path

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.app.data import MapsetLockingException

RUNTIME_GISBASE_SHOULD_BE_PRESENT = "Runtime (GISBASE) should be present"
SESSION_FILE_NOT_DELETED = "Session file not deleted"


def run_in_subprocess(code, tmp_path, env=None):
    """Code as a script in a separate process and return parsed JSON result

    This is useful when we want to ensure that function like init does
    not change the global environment for other tests.
    The current environment is used for the subprocess as is, so any
    existing global session is passed to the subprocess or it needs to
    be removed beforehand, e.g., through the mock_no_session fixture.
    """
    source_file = tmp_path / "test.py"
    source_file.write_text(dedent(code))
    if env is None:
        env = os.environ
    result = subprocess.run(
        [sys.executable, os.fspath(source_file)],
        capture_output=True,
        text=True,
        check=False,
        env=env,
    )
    if result.returncode != 0:
        if result.stderr:
            msg = (
                "Execution of code in subprocess failed, "
                f"captured stderr from subprocess:\n{result.stderr}\n"
            )
        else:
            msg = (
                f"Execution of code in subprocess gave return code {result.returncode}"
                "but there was no stderr"
            )
        raise RuntimeError(msg)
    if not result.stdout:
        msg = "Empty result from subprocess running code"
        raise ValueError(msg)
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError as error:
        msg = f"Invalid JSON: {result.stdout}"
        raise ValueError(msg) from error


@pytest.mark.usefixtures("mock_no_session")
def test_init_as_context_manager(tmp_path):
    """Check that init function's return value works as a context manager"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", flags="p", env=session.env)
        session_file = session.env["GISRC"]
        assert Path(session_file).exists()
    assert not Path(session_file).exists()


@pytest.mark.usefixtures("mock_no_session")
def test_init_session_finish(tmp_path):
    """Check that init works with finish on the returned session object"""
    project = tmp_path / "test"
    gs.create_project(project)
    session = gs.setup.init(project, env=os.environ.copy())
    gs.run_command("g.region", flags="p", env=session.env)
    session_file = session.env["GISRC"]
    session.finish()
    # When operating on the object, the error message should be clear
    # about the session being finished (as opposed to not existing session).
    with pytest.raises(ValueError, match="finished session"):
        session.finish()
    assert not session.active
    assert not Path(session_file).exists()


@pytest.mark.usefixtures("mock_no_session")
def test_global_finish_session_only_in_env(tmp_path):
    """Check that init works with finish on the returned session object"""
    project = tmp_path / "test"
    gs.create_project(project)
    session = gs.setup.init(project, env=os.environ.copy())
    gs.run_command("g.region", flags="p", env=session.env)
    session_file = session.env["GISRC"]
    session.finish()
    # Test preconditions
    assert not session.active
    assert not Path(session_file).exists()
    # The global function can only know that a session does not exist,
    # but not that it existed but has been finished.
    # On top of that, without capured stderr, the error message has
    # only the tool name, but actual error is in stderr.
    with pytest.raises(CalledModuleError, match=r"g\.gisenv"):
        gs.setup.finish(env=session.env)


@pytest.mark.usefixtures("mock_no_session")
def test_init_finish_global_functions_with_env(tmp_path):
    """Check that init and finish global functions work"""
    project = tmp_path / "test"
    gs.create_project(project)
    env = os.environ.copy()
    gs.setup.init(project, env=env)
    gs.run_command("g.region", flags="p", env=env)
    session_file = env["GISRC"]
    gs.setup.finish(env=env)

    assert not Path(session_file).exists()


@pytest.mark.parametrize("capture_stderr", [True, False, None])
@pytest.mark.usefixtures("mock_no_session")
def test_init_finish_global_functions_capture_strerr(tmp_path, capture_stderr):
    """Check init and finish with global env and with set_capture_stderr.

    This actually runs a tool in the created environment and checks its output.
    Capturing or not capturing stderr should not influence the result
    (in is global for grass.script so it may interfere tool calls inside
    grass.script.setup).
    """
    project = tmp_path / "test"
    code = f"""
        import json
        import os
        import grass.script as gs

        gs.set_capture_stderr({capture_stderr})
        gs.create_project(r"{project}")
        gs.setup.init(r"{project}")
        crs_type = gs.parse_command("g.region", flags="p", format="json")["crs"]["type"]
        runtime_present = bool(os.environ.get("GISBASE"))
        gisbase_exists = (
            os.path.exists(os.environ.get("GISBASE")) if runtime_present else False
        )
        result = {{
            "session_file": os.environ["GISRC"],
            "runtime_present": runtime_present,
            "gisbase_exists": gisbase_exists,
            "crs_type": crs_type
        }}
        gs.setup.finish()
        print(json.dumps(result))
    """
    result = run_in_subprocess(code, tmp_path=tmp_path)
    assert result["session_file"], "Expected file name from the subprocess"
    assert result["runtime_present"], RUNTIME_GISBASE_SHOULD_BE_PRESENT
    assert result["gisbase_exists"], "Install directory should exist"
    assert not Path(result["session_file"]).exists(), SESSION_FILE_NOT_DELETED
    assert result["crs_type"] == "xy"


@pytest.mark.usefixtures("mock_no_session")
def test_init_finish_global_functions_runtime_persists(tmp_path):
    """Check that init and finish leave runtime behind.

    This is not necessarily desired behavior and it is not documented that way,
    but it is the current implementation, so we test the expected behavior
    (which may change in the future).
    """
    project = tmp_path / "test"
    code = f"""
        import json
        import os
        import grass.script as gs

        gs.set_capture_stderr(True)
        gs.create_project(r"{project}")
        gs.setup.init(r"{project}")
        region_data = gs.parse_command("g.region", flags="p", format="json")
        runtime_present = bool(os.environ.get("GISBASE"))
        session_file = os.environ["GISRC"]
        gs.setup.finish()
        runtime_present_after = bool(os.environ.get("GISBASE"))
        print(json.dumps({{
            "session_file": session_file,
            "runtime_present": runtime_present,
            "runtime_present_after": runtime_present_after,
            "region_data": region_data
        }}))
    """

    result = run_in_subprocess(code, tmp_path=tmp_path)
    assert result["session_file"], "Expected file name from the subprocess"
    assert result["runtime_present"], RUNTIME_GISBASE_SHOULD_BE_PRESENT
    assert not Path(result["session_file"]).exists(), SESSION_FILE_NOT_DELETED
    # This is testing the current implementation behavior, but it is not required
    # to be this way in terms of design.
    assert result["runtime_present_after"], "Runtime should continue to be present"
    assert result["region_data"]["crs"]["type"] == "xy"


@pytest.mark.usefixtures("mock_no_session")
def test_init_finish_global_functions_set_environment(tmp_path):
    """Check that init and finish global functions work with global env.

    This checks the session without running a tool.
    """
    project = tmp_path / "test"

    code = f"""
        import json
        import os
        import grass.script as gs

        gs.create_project(r"{project}")
        gs.setup.init(r"{project}")
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
        print(json.dumps({{
                "session_file": session_file,
                "session_file_variable_present_during": session_file_variable_present_during,
                "session_file_present_during": session_file_present_during,
                "session_file_variable_present_after": session_file_variable_present_after,
                "runtime_present_during": runtime_present_during,
                "runtime_present_after": runtime_present_after,
                }}
            )
        )
        """
    result = run_in_subprocess(code, tmp_path=tmp_path)

    # Runtime
    assert result["runtime_present_during"], RUNTIME_GISBASE_SHOULD_BE_PRESENT
    # This is testing the current implementation behavior, but it is not required
    # to be this way in terms of design.
    assert result["runtime_present_after"], (
        "Expected GISBASE to be present when finished"
    )

    # Session
    assert result["session_file_present_during"], "Expected session file to be present"
    assert result["session_file_variable_present_during"], (
        "Variable GISRC should be present"
    )
    assert not result["session_file_variable_present_after"], (
        "Not expecting GISRC when finished"
    )
    assert not Path(result["session_file"]).exists(), SESSION_FILE_NOT_DELETED


@pytest.mark.usefixtures("mock_no_session")
def test_init_as_context_manager_env_attribute(tmp_path):
    """Check that session has global environment as attribute"""
    project = tmp_path / "test"
    code = f"""
        import json
        import os
        import grass.script as gs

        gs.create_project(r"{project}")
        with gs.setup.init(r"{project}") as session:
            region_data = gs.parse_command(
                "g.region", flags="p", format="json", env=session.env
            )
            session_file = os.environ["GISRC"]
            runtime_present = bool(os.environ.get("GISBASE"))
            print(json.dumps({{
                "session_file": session_file,
                "file_existed": os.path.exists(session_file),
                "runtime_present": runtime_present,
                "region_data": region_data
            }}))
        """

    result = run_in_subprocess(code, tmp_path=tmp_path)
    assert result["session_file"], "Expected file name from the subprocess"
    assert result["file_existed"], "File should have been present"
    assert result["runtime_present"], RUNTIME_GISBASE_SHOULD_BE_PRESENT
    assert not Path(result["session_file"]).exists(), SESSION_FILE_NOT_DELETED
    assert not os.environ.get("GISRC")
    assert not os.environ.get("GISBASE")
    assert result["region_data"]["crs"]["type"] == "xy"


@pytest.mark.usefixtures("mock_no_session")
def test_init_environment_isolation(tmp_path):
    """Check that only the provided environment is modified"""
    project = tmp_path / "test"
    gs.create_project(project)
    env = os.environ.copy()
    with gs.setup.init(project, env=env) as session:
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


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="Locking is disabled on Windows"
)
def test_init_lock_global_environment(tmp_path):
    """Check that init function can lock a mapset and respect that lock.

    Locking should fail regardless of using the same environment or not.
    Here we are using a global environment as if these would be independent processes.
    """
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        # An attempt to lock a locked mapset should fail.
        with (
            pytest.raises(MapsetLockingException, match=r"Concurrent.*mapset"),
            gs.setup.init(project, env=os.environ.copy(), lock=True),
        ):
            pass


def test_init_ignore_lock_global_environment(tmp_path):
    """Check that no locking ignores the present lock"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        with gs.setup.init(
            project, env=os.environ.copy(), lock=False
        ) as nested_session:
            gs.run_command("g.region", flags="p", env=nested_session.env)
        # No locking is the default.
        with gs.setup.init(project, env=os.environ.copy()) as nested_session:
            gs.run_command("g.region", flags="p", env=nested_session.env)


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="Locking is disabled on Windows"
)
def test_init_lock_nested_environments(tmp_path):
    """Check that init function can lock a mapset using nested environments"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        # An attempt to lock a locked mapset should fail.
        with (
            pytest.raises(MapsetLockingException, match=r"Concurrent.*mapset"),
            gs.setup.init(project, env=top_session.env.copy(), lock=True),
        ):
            pass


def test_init_ignore_lock_nested_environments(tmp_path):
    """Check that No locking ignores the present lock using nested environments"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        with gs.setup.init(
            project, env=top_session.env.copy(), lock=False
        ) as nested_session:
            gs.run_command("g.region", flags="p", env=nested_session.env)
        # No locking is the default.
        with gs.setup.init(project, env=top_session.env.copy()) as nested_session:
            gs.run_command("g.region", flags="p", env=nested_session.env)


def test_init_force_unlock(tmp_path):
    """Force-unlocking should remove an existing lock"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        with gs.setup.init(
            project, env=os.environ.copy(), lock=True, force_unlock=True
        ) as nested_session:
            gs.run_command("g.region", flags="p", env=nested_session.env)


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="Locking is disabled on Windows"
)
def test_init_lock_fail_with_unlock_false(tmp_path):
    """No force-unlocking should fail if there is an existing lock"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        with (
            pytest.raises(MapsetLockingException, match=r"Concurrent.*mapset"),
            gs.setup.init(
                project, env=os.environ.copy(), lock=True, force_unlock=False
            ),
        ):
            pass


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="Locking is disabled on Windows"
)
def test_init_lock_fail_without_unlock(tmp_path):
    """No force-unlocking is the default and it should fail with an existing lock"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        with (
            pytest.raises(MapsetLockingException, match=r"Concurrent.*mapset"),
            gs.setup.init(project, env=os.environ.copy(), lock=True),
        ):
            pass


@pytest.mark.skipif(
    sys.platform.startswith("win"), reason="Locking is disabled on Windows"
)
def test_init_lock_timeout_fail(tmp_path):
    """Fail with locked mapset with non-zero timeout"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy(), lock=True) as top_session:
        gs.run_command("g.region", flags="p", env=top_session.env)
        with (
            pytest.raises(MapsetLockingException, match=r"Concurrent.*mapset"),
            gs.setup.init(project, env=os.environ.copy(), lock=True, timeout=2),
        ):
            pass


@pytest.mark.parametrize("project_path_type", [str, Path])
@pytest.mark.parametrize("grass_path_type", [str, Path])
def test_grass_path_types_in_init(tmp_path, project_path_type, grass_path_type):
    """Check that different path types are accepted as path to grass"""
    # We test with project path type just to be sure there is no interaction.
    project = project_path_type(tmp_path / "test")
    gs.create_project(project)
    grass_path = grass_path_type(gs.setup.get_install_path())
    with gs.setup.init(
        project, grass_path=grass_path, env=os.environ.copy()
    ) as session:
        gs.run_command("g.region", res=1, env=session.env)


@pytest.mark.parametrize("path_type", [str, Path])
def test_grass_path_types_in_setup(tmp_path, path_type):
    """Check that different path types are accepted as path to grass"""
    # We test with project path type just to be sure there is no interaction.
    grass_path = path_type(gs.setup.get_install_path())
    env = os.environ.copy()
    gs.setup.setup_runtime_env(grass_path, env=env)
    # At least before FHS, GISBASE is the way to detect an active runtime.
    assert "GISBASE" in env
