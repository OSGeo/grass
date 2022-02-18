#!/usr/bin/env python3

"""
Script for creating an core_modules_with_last_commit.json file contains
all core modules with their last commit. Used by GitHub "Create new
release draft" action workflow.

JSON file structure:

"r.pack": {
    "commit": "547ff44e6aecfb4c9cbf6a4717fc14e521bec0be",
    "date": "1643883006"
},

commit key value is commit hash
date key value is author date (UNIX timestamp)

Usage:

python utils/generate_core_modules_with_last_commit_json_file.py .

@author Tomas Zigo <tomas.zigo slovanet.sk>
"""

import json
import os
import subprocess
import shutil
import sys


def contains_html_file(files):
    """Contains HTML file

    :param list files: list of files

    :return bool: True if *.html file found
    """
    return ".html" in ",".join(files)


def get_last_commit(src_dir):
    """Generate core modules JSON object with the following structure

    "r.pack": {
        "commit": "547ff44e6aecfb4c9cbf6a4717fc14e521bec0be",
        "date": "1643883006"
    },

    commit key value is commit hash
    date key value is author date (UNIX timestamp)

    :param str src_dir: root source code dir

    :return JSON obj result: core modules with last commit and commit
                             date
    """
    result = {}
    if not shutil.which("git"):
        sys.exit("Git command was not found. Please install it.")
    for root, dirs, files in os.walk(src_dir):
        if not contains_html_file(files):
            continue
        rel_path = os.path.relpath(root)
        process_result = subprocess.run(
            ["git", "log", "-1", "--format=%H,%at", rel_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )  # --format=%H,%at commit hash,author date (UNIX timestamp)
        if process_result.returncode == 0:
            commit, date = process_result.stdout.decode().split(",")
            result[os.path.basename(rel_path)] = {
                "commit": commit,
                "date": date.replace("\n", "").replace("\r", ""),
            }
        else:
            sys.exit(process_result.stderr.decode())
    return result


def main():
    if len(sys.argv) < 2:
        sys.exit("Set root source dir script arg, please.")
    src_dir = sys.argv[1]
    with open(
        os.path.join(
            src_dir,
            "core_modules_with_last_commit.json",
        ),
        "w",
    ) as f:
        json.dump(get_last_commit(src_dir), f, indent=4)


if __name__ == "__main__":
    main()
