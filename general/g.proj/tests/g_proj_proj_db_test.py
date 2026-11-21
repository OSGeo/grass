# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of g.proj behavior when PROJ cannot use its database"""


def test_print_crs_without_proj_db(tools_without_proj_db):
    """The CRS of the current project is printed without proj.db"""
    result = tools_without_proj_db.g_proj(flags="p")
    assert result.returncode == 0, result.stderr
    assert "EPSG" in result.text


def test_interface_description_without_proj_db(tools_without_proj_db):
    """The interface description is available without proj.db"""
    result = tools_without_proj_db.run_cmd(["g.proj", "--interface-description"])
    assert result.returncode == 0, result.stderr
    assert "list_codes" in result.text


def test_list_codes_fails_without_proj_db(tools_without_proj_db):
    """Listing codes ends with an error, not a crash, without proj.db"""
    result = tools_without_proj_db.g_proj(list_codes="EPSG")
    assert result.returncode == 1, result.stderr
