"""Test that grass -c reports a clear error instead of a traceback

SPDX-FileCopyrightText: 2026 Akash Maity
SPDX-FileCopyrightText: GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
"""

import subprocess


def test_missing_database_directory(tmp_path):
    """Missing parent directories give a clear error"""
    project = tmp_path / "missing" / "deeper" / "myproject"
    result = subprocess.run(
        ["grass", "-c", "EPSG:4326", str(project), "-e"],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "Traceback" not in result.stderr
    assert "Cannot create project" in result.stderr
