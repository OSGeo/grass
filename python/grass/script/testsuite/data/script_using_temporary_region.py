#!/usr/bin/env python3

import subprocess
import sys
import os
import platform

import grass.script as gs


def call_use_temp_region(script, size, remaining, nesting, map_name=None):
    pid = os.getpid()
    node = platform.node()
    gs.message(
        "PID {pid} ({node}):"
        " Using {size}x{size} for temporary region on level {nesting}".format(
            **locals()
        )
    )
    # The use_temp_region() function is using program name to generate an
    # identifiable name, so uncomment the following line to trick it and
    # give us even more unique names. (This won't fully test the
    # actual behavior, but it may be useful for debugging).
    # sys.argv[0] = "{script}_nesting_{nesting}".format(**locals())
    gs.use_temp_region()
    gs.run_command("g.region", rows=size, cols=size)
    if remaining:
        nesting += 1
        call = [sys.executable, script, remaining, str(nesting)]
        if map_name:
            call.append(map_name)
        subprocess.check_call(call)
    if map_name:
        gs.run_command(
            "r.mapcalc",
            expression="{map_name}_size_{size}_nesting_{nesting} = 1".format(
                **locals()
            ),
        )


def main():
    this_file = sys.argv[0]
    if len(sys.argv) == 1:
        # Some reasonable defaults for a trivial test to allow calling without
        # any parameters.
        size = 100
        remaining = None
        nesting = 0
        map_name = None
    elif len(sys.argv) not in {3, 4}:
        gs.fatal(
            "Usage: <script name> <size[,size,...]> <nesting level> [<map name>]\n"
            "  <script name>      The name of this file ({name})\n"
            "  <size[,size,...]>  Remaining region sizes\n"
            "  <nesting level>    Nesting level of this call\n"
            "  <map name>         Base name of maps to generate (optional)\n"
            "Examples:\n"
            "  {name} 100 0\n"
            "  {name} 100 1\n"
            "  {name} 100,200 0\n"
            "  {name} 100,200 0 test_raster".format(name=this_file)
        )
    else:
        argument = sys.argv[1]
        sizes = argument.split(",", 1)
        size = sizes[0]
        remaining = sizes[1] if len(sizes) > 1 else None
        nesting = int(sys.argv[2])
        map_name = sys.argv[3] if len(sys.argv) == 4 else None
    call_use_temp_region(
        script=this_file,
        size=size,
        remaining=remaining,
        nesting=nesting,
        map_name=map_name,
    )


if __name__ == "__main__":
    main()
