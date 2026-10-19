"""Fixtures shared by tests of the Python packages

This file is in the python directory rather than in python/grass because
python/grass is a package: pytest would import a conftest.py there as
grass.conftest, and with it the grass package from the source tree instead of
the installed one.
"""

import json
import os
import subprocess
import sys
from textwrap import dedent

import pytest


@pytest.fixture
def clean_subprocess_environment():
    """Environment without a session and without the library search path variable"""
    env = os.environ.copy()
    for variable in (
        "LD_LIBRARY_PATH",
        "DYLD_LIBRARY_PATH",
        "GISRC",
        "GIS_LOCK",
        "GISBASE",
    ):
        env.pop(variable, None)
    return env


@pytest.fixture
def run_code_in_environment(tmp_path):
    """Run Python code in a subprocess with the given environment

    Asserts a zero exit code and returns the parsed JSON from stdout.
    """

    def run(code, env):
        source_file = tmp_path / "subprocess_code.py"
        source_file.write_text(dedent(code))
        result = subprocess.run(
            [sys.executable, os.fspath(source_file)],
            capture_output=True,
            text=True,
            check=False,
            env=env,
        )
        assert result.returncode == 0, (
            result.stderr
            or f"Exit code {result.returncode} without any output (a fatal"
            " error in the GRASS C libraries ends the process without a"
            " traceback)"
        )
        return json.loads(result.stdout)

    return run
