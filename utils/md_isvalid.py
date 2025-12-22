# Check if Markdown produced by --md-description is valid
#
# Requirements:
# - https://github.com/markdownlint/markdownlint
#   (on Debian/Ubuntu - apt install markdownlint)
#   (using Ruby installer - gem install mdl)

import argparse
import os
import subprocess
import sys

import grass.script as gs


def check_md(filename):
    with subprocess.Popen(["mdl", filename]) as p:
        p.wait()


def print_line():
    print("-" * 80)


def check_module(module):
    print_line()
    print(module)
    tmp_file = gs.tempfile()
    with open(tmp_file, "w") as fp:
        with subprocess.Popen([module, "--md-description"], stdout=fp) as p:
            p.wait()
        with subprocess.Popen(
            [
                "mdl",
                "--style",
                os.path.join(os.path.dirname(__file__), "mdl_style.rb"),
                fp.name,
            ]
        ) as p:
            p.wait()
            return p.returncode


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="A tool to check markdown files produced by --md-interface"
    )
    parser.add_argument("-m", "--module", required=False)

    args = parser.parse_args()

    if args.module is None:
        blacklist = ["g.parser"]  # modules with no description
        mcount_failed = 0
        list_cmd = sorted(gs.get_commands()[0])
        for cmd in list_cmd:
            if cmd not in blacklist:
                if check_module(cmd) != 0:
                    mcount_failed += 1

        print_line()
        print("Modules processed {} ({} failed)".format(len(list_cmd), mcount_failed))
        print_line()
        sys.exit(mcount_failed == 0)
    else:
        sys.exit(check_module(args.module))
