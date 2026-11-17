# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of g.proj behavior when PROJ cannot use its database"""


def test_print_crs_without_proj_db(run_g_proj, env_without_proj_db):
    """The CRS of the current project is printed without proj.db"""
    env, _, _ = env_without_proj_db
    returncode, stdout, stderr = run_g_proj(["-p"], env=env)
    assert returncode == 0, stderr
    assert "EPSG" in stdout


def test_interface_description_without_proj_db(run_g_proj, env_without_proj_db):
    """The interface description is available without proj.db"""
    env, _, _ = env_without_proj_db
    returncode, stdout, stderr = run_g_proj(["--interface-description"], env=env)
    assert returncode == 0, stderr
    assert "list_codes" in stdout


def test_list_codes_fails_without_proj_db(run_g_proj, env_without_proj_db):
    """Listing codes ends with an error, not a crash, without proj.db"""
    env, _, _ = env_without_proj_db
    returncode, _, stderr = run_g_proj(["list_codes=EPSG"], env=env)
    assert returncode == 1, stderr
    assert "ERROR:" in stderr
