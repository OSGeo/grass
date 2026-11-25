#!/usr/bin/env python3

"""
Script for testing that the core_modules_with_last_commit.json file
contains every documentation page with its last commit. Used by GitHub
"Additional Checks" action workflow.

Python lib dependencies:

pytest
pytest-depends

Usage:

pytest utils/test_generate_last_commit_file.py

@author Tomas Zigo <tomas.zigo slovanet.sk>
"""

import json
import os
import subprocess
from pathlib import Path

import pytest

from .generate_last_commit_file import COMMIT_DATE_FORMAT


@pytest.fixture
def json_file():
    return "core_modules_with_last_commit.json"


@pytest.fixture
def read_json_file(json_file):
    with open(json_file) as f:
        return json.load(f)


def test_json_file_exists(json_file):
    assert Path(json_file).exists() is True


@pytest.mark.depends(on=["test_json_file_exists"])
def test_json_file_is_not_empty(read_json_file):
    assert len(read_json_file) > 0


# Pairs of documentation page name and the directory its source lives in.
# Besides plain tools, they cover pages whose name differs from their
# directory name and pages which only exist as Markdown, which both rely
# on entries being keyed by page name.
PAGES = [
    ("v.surf.rst", os.path.join("vector", "v.surf.rst")),
    ("r.info", os.path.join("raster", "r.info")),
    ("r3.mapcalc", os.path.join("raster", "r.mapcalc")),
    ("r.watershed", os.path.join("raster", "r.watershed", "front")),
    ("wxGUI.components", os.path.join("gui", "wxpython", "docs")),
    ("databaseintro", "db"),
    ("style_guide", os.path.join("doc", "development")),
    ("python_intro", "doc"),
]


@pytest.mark.depends(on=["test_json_file_is_not_empty"])
@pytest.mark.parametrize(("page", "page_path"), PAGES)
def test_pages_in_json_file(read_json_file, page, page_path):
    assert page in read_json_file


@pytest.mark.depends(
    on=[
        "test_json_file_is_not_empty",
        "test_pages_in_json_file",
    ]
)
@pytest.mark.parametrize(("page", "page_path"), PAGES)
def test_compare_json_file_data(read_json_file, page, page_path):
    # Get Git commit and commit date from local Git
    process_result = subprocess.run(
        [
            "git",
            "log",
            "-1",
            f"--format=%H,{COMMIT_DATE_FORMAT}",
            page_path,
        ],
        capture_output=True,
        check=True,
    )  # --format=%H,COMMIT_DATE_FORMAT commit hash,author date
    commit, date = process_result.stdout.decode().strip().split(",")
    # Compare commit and commit date
    assert read_json_file[page]["commit"] == commit
    assert read_json_file[page]["date"] == date
