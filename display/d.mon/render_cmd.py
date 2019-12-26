#!/usr/bin/env python3
import os
import sys
import tempfile

from grass.script import core as grass
from grass.script import task as gtask
from grass.exceptions import CalledModuleError

# read environment variables from file
def read_env_file(env_file):
    width = height = legfile = None
    fd = open(env_file, 'r')
    if fd is None:
        grass.fatal("Unable to open file '{0}'".format(env_file))
    lines = fd.readlines()
    for l in lines:
        if l.startswith('#'):
            continue
        k, v = l.rstrip('\n').split('#', 1)[0].strip().split('=', 1)
        os.environ[k] = v
        if width is None and k == 'GRASS_RENDER_WIDTH':
            width = int(v)
        if height is None and k == 'GRASS_RENDER_HEIGHT':
            height = int(v)
        if legfile is None and k == 'GRASS_LEGEND_FILE':
            legfile = v
    fd.close()

    if width is None or height is None:
        grass.fatal("Unknown monitor size")

    return width, height, legfile

# run display command
def render(cmd, mapfile):
    env = os.environ.copy()

    if mapfile:
        env['GRASS_RENDER_FILE'] = mapfile
    try:
        grass.run_command(cmd[0], env=env, **cmd[1])
    except CalledModuleError as e:
        grass.debug("Unable to render: {0}".format(e), 1)

# update cmd file
def update_cmd_file(cmd_file, cmd, mapfile):
    if cmd[0] in ('d.colorlist', 'd.font', 'd.fontlist',
                  'd.frame', 'd.info', 'd.mon', 'd.out.file',
                  'd.redraw', 'd.to.rast', 'd.what.rast',
                  'd.what.vect', 'd.where'):
        return

    mode = 'w' if cmd[0] == 'd.erase' else 'a'
    # update cmd file
    fd = open(cmd_file, mode)
    if fd is None:
        grass.fatal("Unable to open file '{0}'".format(cmd_file))
    if mode == 'a':
        frame = os.getenv('GRASS_RENDER_FRAME', None)
        if frame:
            fd.write('# GRASS_RENDER_FRAME={0}\n'.format(frame))
        if mapfile:
            fd.write('# GRASS_RENDER_FILE={0}\n'.format(mapfile))
        fd.write(' '.join(gtask.cmdtuple_to_list(cmd)))
        fd.write('\n')
    else:
        fd.write('')
    fd.close()

# adjust region
def adjust_region(width, height):
    region = grass.region()

    mapwidth = abs(region["e"] - region["w"])
    mapheight = abs(region['n'] - region['s'])

    region["nsres"] = mapheight / height
    region["ewres"] = mapwidth / width
    region['rows'] = int(round(mapheight / region["nsres"]))
    region['cols'] = int(round(mapwidth / region["ewres"]))
    region['cells'] = region['rows'] * region['cols']

    kwdata = [('proj', 'projection'),
              ('zone', 'zone'),
              ('north', 'n'),
              ('south', 's'),
              ('east', 'e'),
              ('west', 'w'),
              ('cols', 'cols'),
              ('rows', 'rows'),
              ('e-w resol', 'ewres'),
              ('n-s resol', 'nsres')]

    grass_region = ''
    for wkey, rkey in kwdata:
        grass_region += '%s: %s;' % (wkey, region[rkey])

    os.environ['GRASS_REGION'] = grass_region

# read any input from stdin and create a temporary file
def read_stdin(cmd):
    opt = None

    if (cmd[0] == 'd.text' and not 'text' in cmd[1] and
        (not 'input' in cmd[1] or cmd[1]['input'] == '-')):
        if sys.stdin.isatty():
            sys.stderr.write("\nPlease enter text instructions.  Enter EOF (ctrl-d) on last line to quit\n")
        opt = 'input'

    if opt:
        tmpfile = tempfile.NamedTemporaryFile(dir=path).name + '.txt'
        fd = open(tmpfile, 'w')
        while 1:
            line = sys.stdin.readline()
            if not line:
                break
            fd.write(line)
        fd.close()
        cmd[1][opt] = tmpfile

if __name__ == "__main__":
    cmd = gtask.cmdstring_to_tuple(sys.argv[1])
    if not cmd[0] or cmd[0] == 'd.mon':
        sys.exit(0)
    path = os.path.dirname(os.path.abspath(__file__))
    mon = os.path.split(path)[-1]

    width, height, legfile = read_env_file(os.path.join(path, 'env'))
    if mon.startswith('wx'):
        mapfile = tempfile.NamedTemporaryFile(dir=path).name
        if cmd[0] in ('d.barscale', 'd.legend', 'd.northarrow', 'd.legend.vect'):
            mapfile += '.png'
        else:
            mapfile += '.ppm'
    else:
        mapfile = None
        adjust_region(width, height)

    read_stdin(cmd)

    render(cmd, mapfile)
    update_cmd_file(os.path.join(path, 'cmd'), cmd, mapfile)
    if cmd[0] == 'd.erase' and os.path.exists(legfile):
        os.remove(legfile)


    sys.exit(0)
