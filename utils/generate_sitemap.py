#!/usr/bin/env python3

# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Generate a sitemap listing every HTML page of a built documentation site.

Static site generators list only the pages they know about, which for
Zensical means the pages in the navigation. The GRASS manual keeps the
hundreds of tool pages out of the navigation, so the sitemap is generated
from the built pages instead.
"""

import argparse
import datetime
from pathlib import Path
from xml.sax.saxutils import escape


def page_paths(site_dir: Path) -> list[str]:
    """Return the site-relative paths of all HTML pages, sorted.

    The error page is not a page to be indexed and is left out.
    """
    return sorted(
        path.relative_to(site_dir).as_posix()
        for path in site_dir.rglob("*.html")
        if path.relative_to(site_dir).as_posix() != "404.html"
    )


def sitemap_xml(site_url: str, paths: list[str], lastmod: datetime.date) -> str:
    """Return the sitemap document for the given page paths."""
    base = site_url.rstrip("/") + "/"
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">',
    ]
    for path in paths:
        lines += [
            "  <url>",
            f"    <loc>{escape(base + path)}</loc>",
            f"    <lastmod>{lastmod.isoformat()}</lastmod>",
            "  </url>",
        ]
    lines.append("</urlset>")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--site-dir",
        type=Path,
        required=True,
        help="directory with the built HTML pages",
    )
    parser.add_argument(
        "--site-url",
        required=True,
        help="URL the site directory is published at",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="output file (default: sitemap.xml in the site directory)",
    )
    args = parser.parse_args()

    if not args.site_dir.is_dir():
        parser.error(f"{args.site_dir} is not a directory")
    output = args.output or args.site_dir / "sitemap.xml"

    paths = page_paths(args.site_dir)
    output.write_text(
        sitemap_xml(args.site_url, paths, datetime.date.today()), encoding="utf-8"
    )
    print(f"Wrote {len(paths)} pages to {output}")


if __name__ == "__main__":
    main()
