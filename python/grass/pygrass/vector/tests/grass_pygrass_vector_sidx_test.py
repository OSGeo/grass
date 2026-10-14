# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test that closing a vector map opened in update mode releases the sidx file

An open file handle left behind makes the removal of the map fail on Windows
and leaks a file descriptor on all platforms.
"""

import os
import subprocess
import sys
from pathlib import Path

import pytest

import grass.script as gs

CHECKS = Path(__file__).with_name("grass_pygrass_vector_sidx_checks.py")


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Active session in an XY project (scope: module)"""
    project = tmp_path_factory.mktemp("sidx") / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


def test_update_mode_releases_sidx(session):
    """The sidx file is closed after a map opened with "rw" is closed"""
    env = session.env.copy()
    env["MAPSET_PATH"] = str(
        Path(gs.gisenv(env=session.env)["GISDBASE"])
        / gs.gisenv(env=session.env)["LOCATION_NAME"]
        / gs.gisenv(env=session.env)["MAPSET"]
    )
    result = subprocess.run(
        [sys.executable, str(CHECKS)],
        env=env,
        capture_output=True,
        text=True,
        check=False,
        timeout=120,
    )
    assert result.returncode == 0, f"{result.stdout}\n{result.stderr}"
