#!/usr/bin/env python3

"""Tests for the URL version handling in utils/merge_sitemaps.py.

Usage:

    pytest utils/test_merge_sitemaps.py

@author: Corey White
"""

import pytest

from .merge_sitemaps import check_url_version

BASE = "https://grass.osgeo.org"


@pytest.mark.parametrize("version", [None, ""])
def test_preserves_url_when_no_version(version):
    """A falsy version leaves the URL untouched so each sitemap keeps its prefix."""
    url = f"{BASE}/grass86/manuals/libpython/grass.html"
    assert check_url_version(url, version) == url


def test_rewrites_first_path_segment():
    url = f"{BASE}/grass-stable/manuals/r.info.html"
    expected = f"{BASE}/grass86/manuals/r.info.html"
    assert check_url_version(url, "grass86") == expected


def test_noop_when_segment_already_matches():
    url = f"{BASE}/grass86/manuals/libpython/grass.html"
    assert check_url_version(url, "grass86") == url


def test_ignores_url_without_a_path_segment():
    url = f"{BASE}/"
    assert check_url_version(url, "grass86") == url
