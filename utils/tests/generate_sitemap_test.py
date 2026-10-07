# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests for the sitemap generation from a built site."""

import datetime
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

UTILS_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(UTILS_DIR))

import generate_sitemap  # noqa: E402

NS = "{http://www.sitemaps.org/schemas/sitemap/0.9}"


@pytest.fixture
def site_dir(tmp_path):
    """Built site with pages at the root and in a subdirectory, plus non-pages."""
    for name in [
        "index.html",
        "r.slope.aspect.html",
        "404.html",
        "addons/r.tree.html",
        "assets/javascripts/bundle.js",
        "colortables/aspect.png",
        "search.json",
    ]:
        path = tmp_path / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("")
    return tmp_path


def test_page_paths(site_dir):
    assert generate_sitemap.page_paths(site_dir) == [
        "addons/r.tree.html",
        "index.html",
        "r.slope.aspect.html",
    ]


@pytest.mark.parametrize(
    "site_url",
    ["https://example.org/manuals", "https://example.org/manuals/"],
)
def test_sitemap_xml(site_url):
    xml = generate_sitemap.sitemap_xml(
        site_url, ["index.html", "addons/r.tree.html"], datetime.date(2026, 9, 22)
    )
    urls = ET.fromstring(xml).findall(f"{NS}url")
    assert [url.find(f"{NS}loc").text for url in urls] == [
        "https://example.org/manuals/index.html",
        "https://example.org/manuals/addons/r.tree.html",
    ]
    assert {url.find(f"{NS}lastmod").text for url in urls} == {"2026-09-22"}


def test_sitemap_xml_escapes_url():
    xml = generate_sitemap.sitemap_xml(
        "https://example.org/a&b", ["x.html"], datetime.date(2026, 9, 22)
    )
    assert ET.fromstring(xml).find(f"{NS}url/{NS}loc").text == (
        "https://example.org/a&b/x.html"
    )


def test_script_writes_sitemap_into_site_dir(site_dir):
    subprocess.run(
        [
            sys.executable,
            str(UTILS_DIR / "generate_sitemap.py"),
            "--site-dir",
            str(site_dir),
            "--site-url",
            "https://example.org/manuals/",
        ],
        check=True,
    )
    locs = [
        loc.text
        for loc in ET.parse(site_dir / "sitemap.xml").getroot().iter(f"{NS}loc")
    ]
    assert locs == [
        "https://example.org/manuals/addons/r.tree.html",
        "https://example.org/manuals/index.html",
        "https://example.org/manuals/r.slope.aspect.html",
    ]
