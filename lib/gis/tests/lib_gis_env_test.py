"""Test environment and GIS environment functions"""

import json
import os
import subprocess
import sys
from textwrap import dedent

import pytest

import grass.script as gs


def run_in_subprocess(code, tmp_path, env):
    """Code as a script in a separate process and return parsed JSON result

    This is useful when we want to ensure that function like init does
    not change the global environment for other tests.
    Some effort is made to remove a current if any, but it does not touch
    runtime variables on purpose to enable dynamic library loading based on
    a path variable.
    """
    source_file = tmp_path / "test.py"
    source_file.write_text(dedent(code))
    env = env.copy()
    for variable in ("GISRC", "GIS_LOCK"):
        if variable in env:
            del env[variable]
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


# To read the new variables on Windows, another subprocess would be needed,
# but that would not allow for the dynamic changes of the session variables within
# a single process which is what this test is testing.
@pytest.mark.skipif(
    sys.platform.startswith("win"),
    reason="On Windows, C has a cached environment, so the libraries don't know about the session.",
)
def test_reading_respects_change_of_session(tmp_path):
    """Check new session file path is retrieved and the file is read"""

    # The switching must happen in one process to test that the reading functions
    # are not using the cached values. However, the test itself needs to be
    # process-separated otherwise other tests will be influenced by the loaded
    # libraries and initialized data structures.

    code = f"""
        # Switches through a list of projects
        import json
        from pathlib import Path
        import grass.script as gs
        import grass.pygrass.utils as pygrass_utils
        import grass.lib.gis as libgis

        names = []
        for project_name in ["test1", "test2", "abc"]:
            gs.create_project(Path(r"{tmp_path}") / project_name)
            with gs.setup.init(Path(r"{tmp_path}") / project_name):
                libgis.G__read_gisrc_path()
                libgis.G__read_gisrc_env()
                names.append((pygrass_utils.getenv("LOCATION_NAME"), project_name))
        print(json.dumps(names))
        """

    gs.create_project(tmp_path / "base")
    with gs.setup.init(tmp_path / "base", env=os.environ.copy()) as session:
        names = run_in_subprocess(code, tmp_path=tmp_path, env=session.env)

    for getenv_name, expected_name in names:
        assert getenv_name == expected_name, f"All recorded names: {names}"
