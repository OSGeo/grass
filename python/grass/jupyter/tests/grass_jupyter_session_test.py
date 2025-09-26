"""Test session functions in grass.jupyter"""

import subprocess
import os
import sys


# All init tests change the global environment, but we use a separate process
# only when it is necessary.
# Ideally, the functions would support env parameter and the tests
# would mostly use that.
def run_in_subprocess(file):
    """Run function in a separate process
    The function must take a Queue and put its result there.
    The result is then returned from this function.
    """
    process = subprocess.run(
        [sys.executable, os.fspath(file)], stdout=subprocess.PIPE, text=True, check=True
    )
    return process.stdout


def test_init_finish(tmp_path):
    """Check that init function works with an explicit session finish"""
    project = tmp_path / "test"
    script = f"""
import os
import grass.script as gs
import grass.jupyter as gj
gs.create_project(r"{project}")
session = gj.init(r"{project}")
gs.read_command("g.region", flags="p")
print(os.environ["GISRC"])
session.finish()
"""
    file = tmp_path / "script.py"
    file.write_text(script)
    session_file = run_in_subprocess(file)
    assert session_file, "Expected something from the subprocess"
    session_file = session_file.strip()
    assert "\n" not in session_file, "Expected a file name from the subprocess"
    assert not os.path.exists(session_file), f"Session file {session_file} not deleted"


def test_init_with_auto_finish(tmp_path):
    """Check that init function works with an implicit session finish"""
    project = tmp_path / "test"
    script = f"""
import os
import grass.script as gs
import grass.jupyter as gj
gs.create_project(r"{project}")
session = gj.init(r"{project}")
print(os.environ["GISRC"])
"""

    file = tmp_path / "script.py"
    file.write_text(script)
    session_file = run_in_subprocess(file)
    assert session_file, "Expected something from the subprocess"
    session_file = session_file.strip()
    assert "\n" not in session_file, "Expected a file name from the subprocess"
    assert not os.path.exists(session_file), f"Session file {session_file} not deleted"
