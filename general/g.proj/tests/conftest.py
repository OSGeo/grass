# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Fixtures for g.proj tests"""

import os
import subprocess

import pytest

import grass.script as gs


def _run_g_proj(args, env):
    process = gs.Popen(
        ["g.proj", *args],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        # Paths with non-ASCII characters may not be in the expected encoding.
        errors="replace",
    )
    stdout, stderr = process.communicate()
    return process.returncode, stdout, stderr


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("grass_session")
    project = "test_project"
    gs.create_project(tmp_path, project, epsg="3358")
    with gs.setup.init(tmp_path / project, env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def run_g_proj():
    """Function which runs g.proj and returns the return code, stdout, and stderr

    It takes a list of arguments and an environment.
    """
    return _run_g_proj


@pytest.fixture
def env_without_proj_db(session, tmp_path):
    """Environment in which PROJ cannot find proj.db

    PROJ looks into the user-writable directory first and, when PROJ_DATA
    is set, only into the directories listed there, so pointing both to
    empty directories hides every proj.db on the machine. PROJ_LIB is set
    to the same directory for PROJ 9.0 which uses only PROJ_LIB.

    Returns the environment, the PROJ_DATA directory, and the user directory.
    """
    env = session.env.copy()
    proj_data = tmp_path / "empty_proj_data"
    proj_data.mkdir()
    user_dir = tmp_path / "empty_user_dir"
    user_dir.mkdir()
    env["PROJ_DATA"] = str(proj_data)
    env["PROJ_LIB"] = str(proj_data)
    env["PROJ_USER_WRITABLE_DIRECTORY"] = str(user_dir)
    returncode, _, _ = _run_g_proj(["-w", "epsg=3358"], env=env)
    if returncode == 0:
        pytest.skip("PROJ database cannot be hidden (embedded in PROJ)")
    return env, proj_data, user_dir
