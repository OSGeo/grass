#!/usr/bin/env python3
# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Convert Markdown pages without an up-to-date man page.

Used by the CMake build after the index pages are generated. Tool pages
already converted during the tool builds are up to date and are skipped,
so this effectively converts the index pages, including the dynamically
named topic_* pages. Web-only pages, which have no man page equivalent,
are passed with --exclude.
"""

import argparse
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("md_dir", type=Path, help="directory with Markdown pages")
    parser.add_argument("man_dir", type=Path, help="directory for man pages")
    parser.add_argument("md2man", help="path to g.md2man.py")
    parser.add_argument(
        "--exclude",
        nargs="*",
        default=[],
        metavar="NAME",
        help="page names (without extension) to skip",
    )
    args = parser.parse_args()

    args.man_dir.mkdir(parents=True, exist_ok=True)
    for md_file in sorted(args.md_dir.glob("*.md")):
        if md_file.stem in args.exclude:
            continue
        man_file = args.man_dir / (md_file.stem + ".1")
        # Convert also on equal timestamps: with coarse mtime granularity
        # a page regenerated within the same second would otherwise be
        # skipped, and a redundant conversion is cheaper than a stale page.
        if man_file.exists() and man_file.stat().st_mtime > md_file.stat().st_mtime:
            continue
        subprocess.run(
            [sys.executable, args.md2man, str(md_file), str(man_file)], check=True
        )


if __name__ == "__main__":
    main()
