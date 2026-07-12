#!/usr/bin/env python3
# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Convert Markdown pages without an up-to-date man page.

Used by the CMake build after the index pages are generated. Tool pages
already converted during the tool builds are up to date and are skipped,
so this effectively converts the index pages, including the dynamically
named topic_* pages.

Usage: build_manpages.py <markdown-directory> <man-directory> <g.md2man.py>
"""

import subprocess
import sys
from pathlib import Path


def main():
    md_dir = Path(sys.argv[1])
    man_dir = Path(sys.argv[2])
    md2man = sys.argv[3]

    man_dir.mkdir(parents=True, exist_ok=True)
    for md_file in sorted(md_dir.glob("*.md")):
        man_file = man_dir / (md_file.stem + ".1")
        if man_file.exists() and man_file.stat().st_mtime >= md_file.stat().st_mtime:
            continue
        subprocess.run(
            [sys.executable, md2man, str(md_file), str(man_file)], check=True
        )


if __name__ == "__main__":
    main()
