# Check if Markdown produced by --md-description is valid
#
# Requirements:
# - https://github.com/markdownlint/markdownlint
#   (on Debian/Ubuntu - apt install ruby-mdl)

import sys
import argparse
import subprocess

import grass.script as gs


def check_md(filename):
    p = subprocess.Popen(["mdl", filename])
    p.wait()


def check_module(module):
    print("-" * 80)
    print(module)
    print("-" * 80)
    tmp_file = gs.tempfile()
    with open(tmp_file, 'w') as fp:
        p = subprocess.Popen([module, "--md-description"], stdout=fp)
        p.wait()

        p = subprocess.Popen(["mdl", fp.name])
        p.wait()

        returncode = p.returncode

    return returncode


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="A tool to check markdown files produced by --md-interface"
    )
    parser.add_argument("-m", "--module", required=False)

    args = parser.parse_args()

    if args.module is None:
        for cmd in gs.get_commands()[0]:
            check_module(cmd)
    else:
        returncode = check_module(args.module)

    sys.exit(returncode)
