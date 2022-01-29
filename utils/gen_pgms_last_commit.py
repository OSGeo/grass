#!/usr/bin/env python3
"""
Script for creating an pgms_with_last_commit.json file contains all pgms
with their last commit
{"pgm": {"commit": "git log -1 PGM src dir"}}.

Used by GitHub actions workflow.

python gen_pgms_last_commit.py src_root_dir

@author Tomas Zigo <tomas.zigo slovanet.sk>
"""

import json
import os
import subprocess
import shutil
import sys


def get_last_commit(src_dir):
    """Generate JSON object with the following structure e.g.
    {"pgm": {"commit": "git log -1 PGM src dir"}}

    :param str src_dir: root src dir

    :return JSON obj result: JSON object
    """
    result = {}
    if not shutil.which("git"):
        sys.exit("Git command was not found. Please install it.")
    for root, dirs, files in os.walk(src_dir):
        for f in files:
            if f.endswith(".html"):
                rel_path = os.path.relpath(root)
                stdout, stderr = subprocess.Popen(
                    ["git", "log", "-1", rel_path],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                ).communicate()
                if stderr:
                    sys.exit(stderr.decode())
                if stdout:
                    result[os.path.basename(rel_path)] = {
                        "commit": stdout.decode(),
                    }
    return result


def main():
    if len(sys.argv) < 2:
        sys.exit("Set root source dir script arg, please.")
    src_dir = sys.argv[1]
    with open(os.path.join(src_dir, "pgms_with_last_commit.json"), "w") as f:
        json.dump(get_last_commit(src_dir), f, indent=4)


if __name__ == "__main__":
    main()
