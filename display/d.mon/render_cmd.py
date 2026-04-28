#!/usr/bin/env python3
import glob
import os
import sys
import tempfile
from pathlib import Path

from grass.exceptions import CalledModuleError
from grass.script import core as grass
from grass.script import task as gtask

non_rendering_modules = (
    "d.colorlist",
    "d.font",
    "d.fontlist",
    "d.frame",
    "d.info",
    "d.mon",
    "d.out.file",
    "d.to.rast",
    "d.what.rast",
    "d.what.vect",
    "d.where",
)


# remove empty mapfile from non-rendering modules
def remove_mapfile(mapfile):
    # adopted from Map.DeleteLayer() in gui/wxpython/core/render.py
    base, mapfile = os.path.split(mapfile)
    tempbase = mapfile.split(".")[0]
    if base == "" or tempbase == "":
        return
    basefile = os.path.join(base, tempbase) + r".*"
    for f in glob.glob(basefile):
        os.remove(f)


# read environment variables from file
def read_env_file(env_file):
    width = height = legfile = None
    try:
        with open(env_file) as fd:
            for line in fd:
                line = line.strip()
                if line.startswith("#") or not line:
                    continue
                # Split on comment and parse key=value
                k, v = line.split("#", 1)[0].strip().split("=", 1)
                os.environ[k] = v
                if width is None and k == "GRASS_RENDER_WIDTH":
                    width = int(v)
                if height is None and k == "GRASS_RENDER_HEIGHT":
                    height = int(v)
                if legfile is None and k == "GRASS_LEGEND_FILE":
                    legfile = v
    except OSError:
        grass.fatal(f"Unable to open file '{env_file}'")

    if width is None or height is None:
        grass.fatal("Unknown monitor size")

    return width, height, legfile


# run display command
def render(cmd, mapfile):
    env = os.environ.copy()

    if mapfile:
        env["GRASS_RENDER_FILE"] = mapfile
    try:
        grass.run_command(cmd[0], env=env, **cmd[1])
        # display driver can generate a blank map file unnecessarily for
        # non-rendering modules; delete it
        if cmd[0] in non_rendering_modules and Path(mapfile).exists():
            remove_mapfile(mapfile)

    except CalledModuleError as e:
        grass.debug("Unable to render: {0}".format(e), 1)


# update cmd file
def update_cmd_file(cmd_file, cmd, mapfile):
    if cmd[0] in non_rendering_modules:
        return

    mode = "w" if cmd[0] == "d.erase" else "a"
    # update cmd file
    try:
        with open(cmd_file, mode) as fd:
            if mode == "a":
                frame = os.getenv("GRASS_RENDER_FRAME", None)
                if frame:
                    fd.write("# GRASS_RENDER_FRAME={0}\n".format(frame))
                if mapfile:
                    fd.write("# GRASS_RENDER_FILE={0}\n".format(mapfile))
                fd.write(" ".join(gtask.cmdtuple_to_list(cmd)))
                fd.write("\n")
            else:
                fd.write("")
    except OSError:
        grass.fatal(f"Unable to open file '{cmd_file}'")


# adjust region
def adjust_region(width, height):
    region = grass.region()

    mapwidth = abs(region["e"] - region["w"])
    mapheight = abs(region["n"] - region["s"])

    region["nsres"] = mapheight / height
    region["ewres"] = mapwidth / width
    region["rows"] = round(mapheight / region["nsres"])
    region["cols"] = round(mapwidth / region["ewres"])
    region["cells"] = region["rows"] * region["cols"]

    kwdata = [
        ("proj", "projection"),
        ("zone", "zone"),
        ("north", "n"),
        ("south", "s"),
        ("east", "e"),
        ("west", "w"),
        ("cols", "cols"),
        ("rows", "rows"),
        ("e-w resol", "ewres"),
        ("n-s resol", "nsres"),
    ]

    grass_region = ""
    for wkey, rkey in kwdata:
        grass_region += "%s: %s;" % (wkey, region[rkey])

    os.environ["GRASS_REGION"] = grass_region


# read any input from stdin and create a temporary file
def read_stdin(cmd):
    opt = None

    if (
        cmd[0] == "d.text"
        and "text" not in cmd[1]
        and ("input" not in cmd[1] or cmd[1]["input"] == "-")
    ):
        if sys.stdin.isatty():
            sys.stderr.write(
                "\nPlease enter text instructions."
                " Enter EOF (ctrl-d) on last line to quit.\n"
            )
        opt = "input"

    if opt:
        with tempfile.NamedTemporaryFile(
            dir=path, suffix=".txt", mode="w", delete=False
        ) as fd:
            while True:
                line = sys.stdin.readline()
                if not line:
                    break
                fd.write(line)
            tmpfile = fd.name
        cmd[1][opt] = tmpfile


if __name__ == "__main__":
    cmd = gtask.cmdstring_to_tuple(sys.argv[1])
    if not cmd[0] or cmd[0] == "d.mon":
        sys.exit(0)
    path = os.path.dirname(os.path.abspath(__file__))
    mon = os.path.split(path)[-1]

    width, height, legfile = read_env_file(os.path.join(path, "env"))
    if mon.startswith("wx"):
        if cmd[0] in {"d.barscale", "d.legend", "d.northarrow", "d.legend.vect"}:
            suffix = ".png"
        else:
            suffix = ".ppm"

        with tempfile.NamedTemporaryFile(dir=path, suffix=suffix) as tmpfile:
            mapfile = tmpfile.name
        # to force rendering by wx monitors, but don't create a map file for
        # non-rendering modules
        if cmd[0] not in non_rendering_modules:
            Path(mapfile).touch()
    else:
        mapfile = None
        adjust_region(width, height)

    read_stdin(cmd)

    # wx monitors will render new layers internally so don't render them here;
    # also, some display modules print information to the terminal rather than
    # rendering any contents on the monitor so allow them to run here
    if not mon.startswith("wx") or cmd[0] in non_rendering_modules:
        render(cmd, mapfile)

    update_cmd_file(os.path.join(path, "cmd"), cmd, mapfile)
    if cmd[0] == "d.erase" and Path(legfile).exists():
        os.remove(legfile)

    sys.exit(0)
