# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests for the encoding of pages built by mkmarkdown.py and mkhtml.py."""

import os
import subprocess
import sys
from pathlib import Path

import pytest

UTILS_DIR = Path(__file__).resolve().parent.parent

# The UTF-8 encoding of these characters contains bytes which are undefined
# in cp1252, the usual encoding of the locale on Windows.
TEXT = "Dvořák and Černý: “10 °C → 20 °C”"


@pytest.mark.parametrize(
    ("script", "ext"), [("mkmarkdown.py", "md"), ("mkhtml.py", "html")]
)
def test_page_is_utf8_without_utf8_locale(tmp_path, script, ext):
    """Check that sources are read and the page written as UTF-8

    The scripts run during the build of an addon on the user's machine
    where the encoding of the locale can be anything.
    """
    tool_dir = tmp_path / "raster" / "r.tool"
    tool_dir.mkdir(parents=True)
    (tool_dir / f"r.tool.{ext}").write_text(f"{TEXT} in source\n", encoding="utf-8")
    (tool_dir / f"r.tool.tmp.{ext}").write_text(
        f"{TEXT} in interface\n", encoding="utf-8"
    )
    env = os.environ.copy()
    for name in ("SOURCE_URL", "PYTHONIOENCODING"):
        env.pop(name, None)
    env.update(
        {
            "MODULE_TOPDIR": str(tmp_path),
            "VERSION_NUMBER": "8.6.0",
            # Use the encoding of the locale and make that encoding ASCII
            # where the platform allows it.
            "PYTHONUTF8": "0",
            "PYTHONCOERCECLOCALE": "0",
            "LC_ALL": "C",
        }
    )
    result = subprocess.run(
        [sys.executable, str(UTILS_DIR / script), "r.tool"],
        cwd=tool_dir,
        env=env,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr.decode(errors="replace")
    page = result.stdout.decode("utf-8")
    assert f"{TEXT} in source" in page
    assert f"{TEXT} in interface" in page
