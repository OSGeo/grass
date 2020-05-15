#!/usr/bin/env python3

import subprocess
import sys
import os
import platform

import grass.script as gs


def call_use_temp_region(script, size, remaining, nesting):
    pid = os.getpid()
    node = platform.node()
    gs.message(
        "PID {pid} ({node}):"
        " Using {size}x{size} for temporary region on level {nesting}".format(
            **locals()
        )
    )
    # The use_temp_region() function is using program name to generate an
    # identifiable name, so uncomment the following like to trick it and
    # give us even more unique names (changes the nature test, but may be
    # useful for debugging).
    # sys.argv[0] = "{this_file}_nesting_{nesting}".format(**locals())
    gs.use_temp_region()
    gs.run_command("g.region", rows=size, cols=size)
    if remaining:
        nesting += 1
        subprocess.check_call([sys.executable, script, remaining, str(nesting)])


def main():
    this_file = sys.argv[0]
    if len(sys.argv) == 1:
        # Some resonable defaults for a trivial test to allow calling without
        # any parameters.
        size = 100
        remaining = None
        nesting = 0
    elif len(sys.argv) < 3:
        gs.fatal(
            "Usage: <script name> <size[,size,...]> <nesting level>\n"
            "<script name>      The name of this file ({})\n"
            "<size[,size,...]>  Remaining region sizes\n"
            "<nesting level>    Nesting level of this call".format(this_file)
        )
    else:
        argument = sys.argv[1]
        sizes = argument.split(",", 1)
        size = sizes[0]
        if len(sizes) > 1:
            remaining = sizes[1]
        else:
            remaining = None
        nesting = int(sys.argv[2])
    call_use_temp_region(
        script=this_file, size=size, remaining=remaining, nesting=nesting
    )


if __name__ == "__main__":
    main()
