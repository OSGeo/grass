"""Test that grass -c reports a clear error instead of a traceback

SPDX-FileCopyrightText: 2026 Akash Maity
SPDX-FileCopyrightText: GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
"""

import os
import subprocess


def get_grass_executable():
    """Return the command that starts GRASS on this operating system"""
    if os.name != "nt":
        return "grass"
    gisbase = os.environ.get("GISBASE")
    if gisbase:
        return os.path.join(gisbase, "grass.bat")
    return "grass.bat"


def test_missing_database_directory(tmp_path):
    """Missing parent directories give a clear error"""
    project = tmp_path / "missing" / "deeper" / "myproject"
    result = subprocess.run(
        [get_grass_executable(), "-c", "EPSG:4326", str(project), "-e"],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "Traceback" not in result.stderr
    assert "Cannot create project" in result.stderr
