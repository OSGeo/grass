#!/usr/bin/env python3

"""
Script for creating an core_modules_with_last_commit.json file contains
all core modules with their last commit. Used by GitHub "Create new
release draft" action workflow.

JSON file structure:

"r.pack": {
    "commit": "547ff44e6aecfb4c9cbf6a4717fc14e521bec0be",
    "date": "2022-02-20T09:34:17+01:00"
},

commit key value is commit hash
date key value is author date

Usage:

python utils/generate_last_commit_file.py .

@author Tomas Zigo <tomas.zigo slovanet.sk>
"""

import json
import os
import subprocess
import shutil
import sys


# Strict ISO 8601 format
COMMIT_DATE_FORMAT = "%aI"


def get_last_commit(src_dir):
    """Generate core modules JSON object with the following structure

    "r.pack": {
        "commit": "547ff44e6aecfb4c9cbf6a4717fc14e521bec0be",
        "date": "2022-02-20T09:34:17+01:00"
    },

    commit key value is commit hash
    date key value is author date

    :param str src_dir: root source code dir

    :return JSON obj result: core modules with last commit and commit
                             date
    """
    result = {}
    join_sep = ","
    if not shutil.which("git"):
        sys.exit("Git command was not found. Please install it.")
    for root, _, files in os.walk(src_dir):
        if ".html{}".format(join_sep) not in join_sep.join(files) + join_sep:
            continue
        rel_path = os.path.relpath(root)
        process_result = subprocess.run(
            [
                "git",
                "log",
                "-1",
                f"--format=%H,{COMMIT_DATE_FORMAT}",
                rel_path,
            ],
            capture_output=True,
        )  # --format=%H,COMMIT_DATE_FORMAT commit hash,author date
        if process_result.returncode == 0:
            try:
                text = process_result.stdout.decode().strip()
                if not text:
                    # Non-versioned directories are picked by the filter, but git log
                    # returns nothing which is fine, so silently skipping these.
                    continue
                commit, date = text.split(",")
            except ValueError as error:
                sys.exit(
                    f"Cannot parse output from git log for '{rel_path}': "
                    f"{text} because {error}"
                )
            result[os.path.basename(rel_path)] = {
                "commit": commit,
                "date": date,
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
