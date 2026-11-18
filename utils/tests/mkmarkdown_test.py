# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests for the Markdown page assembly in mkmarkdown.py."""

import importlib
import sys
from pathlib import Path

import pytest

UTILS_DIR = Path(__file__).resolve().parent.parent


@pytest.fixture(scope="module")
def mkmarkdown():
    """Import mkmarkdown the way the build runs it and clean up afterwards"""
    with pytest.MonkeyPatch.context() as patch:
        patch.syspath_prepend(str(UTILS_DIR))
        # The shared helpers resolve the repository root from the build
        # environment at import time, so they need it set even when
        # the functions under test never use it.
        patch.setenv("MODULE_TOPDIR", str(UTILS_DIR.parent))
        # The helpers are in utils/mkdocs.py, which must not stay imported
        # under the name of the MkDocs package for the rest of the session.
        patch.delitem(sys.modules, "mkdocs", raising=False)
        yield importlib.import_module("mkmarkdown")
        for name in ("mkmarkdown", "mkdocs"):
            sys.modules.pop(name, None)


@pytest.mark.parametrize(
    ("url_source", "pgm", "expected"),
    [
        (
            "https://github.com/OSGeo/grass/tree/main/raster/r.slope.aspect",
            "r.slope.aspect",
            "https://github.com/OSGeo/grass/edit/main/raster/r.slope.aspect/r.slope.aspect.md",
        ),
        (
            "https://github.com/OSGeo/grass/tree/main/gui/wxpython/docs/",
            "wxGUI.nviz",
            "https://github.com/OSGeo/grass/edit/main/gui/wxpython/docs/wxGUI.nviz.md",
        ),
        (
            "https://github.com/OSGeo/grass-addons/tree/grass8/src/raster/r.tree",
            "r.tree",
            "https://github.com/OSGeo/grass-addons/edit/grass8/src/raster/r.tree/r.tree.md",
        ),
    ],
)
def test_source_edit_url(mkmarkdown, url_source, pgm, expected):
    assert mkmarkdown.source_edit_url(url_source, pgm) == expected


@pytest.mark.parametrize(
    "url_source", ["", "raster/r.slope.aspect", "https://example.com/user/repo/r.tool"]
)
def test_source_edit_url_not_repository_tree(mkmarkdown, url_source):
    assert mkmarkdown.source_edit_url(url_source, "r.tool") is None


def test_merge_md_files_appends_extra_yaml(mkmarkdown):
    generated = "---\nname: r.tool\n---\n\n# r.tool\n"
    source = "---\ndescription: Tool\n---\n\nBody\n"
    result = "".join(
        mkmarkdown.merge_md_files(
            generated, source, None, None, extra_yaml="source_edit_url: URL"
        )
    )
    assert result.startswith(
        "---\nname: r.tool\ndescription: Tool\nsource_edit_url: URL\n---\n"
    )
    assert "Body" in result


def test_merge_md_files_extra_yaml_creates_header(mkmarkdown):
    result = "".join(
        mkmarkdown.merge_md_files("", "# Page\n", None, None, extra_yaml="key: value")
    )
    assert result.startswith("---\nkey: value\n---\n")
