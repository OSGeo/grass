#!/usr/bin/env python
import os
import sys
import tempfile

from grass.script import core as grass
from grass.script import task as gtask

# read environment variables from file
def read_env_file(env_file):
    width = height = None
    fd = open(env_file, 'r')
    if fd is None:
        grass.fatal("Unable to open file '{}'".format(env_file))
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
    fd.close()
    
    if width is None or height is None:
        grass.fatal("Unknown monitor size")
    
    return width, height

# run display command
def render(cmd, mapfile):
    env = os.environ.copy()
    if mapfile:
        env['GRASS_RENDER_FILE'] = mapfile
    try:
        grass.run_command(cmd[0], env=env, **cmd[1])
    except Exception as e:
        grass.debug(1, "Unable to render: {}".format(e))

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
        grass.fatal("Unable to open file '{}'".format(cmd_file))
    if mode == 'a':
        frame = os.getenv('GRASS_RENDER_FRAME', None)
        if frame:
            fd.write('# GRASS_RENDER_FRAME={}\n'.format(frame))
        if mapfile:
            fd.write('# GRASS_RENDER_FILE={}\n'.format(mapfile))
        fd.write(' '.join(gtask.cmdtuple_to_list(cmd)))
        fd.write('\n')
    else:
         fd.write('')
    fd.close()

# adjust region
def adjust_region(width, height):
    region = grass.region()
    
    mapwidth  = abs(region["e"] - region["w"])
    mapheight = abs(region['n'] - region['s'])
    
    region["nsres"] =  mapheight / height
    region["ewres"] =  mapwidth  / width
    region['rows']  = int(round(mapheight / region["nsres"]))
    region['cols']  = int(round(mapwidth / region["ewres"]))
    region['cells'] = region['rows'] * region['cols']
    
    kwdata = [('proj',      'projection'),
              ('zone',      'zone'),
              ('north',     'n'),
              ('south',     's'),
              ('east',      'e'),
              ('west',      'w'),
              ('cols',      'cols'),
              ('rows',      'rows'),
              ('e-w resol', 'ewres'),
              ('n-s resol', 'nsres')]
    
    grass_region = ''
    for wkey, rkey in kwdata:
        grass_region += '%s: %s;' % (wkey, region[rkey])
    
    os.environ['GRASS_REGION'] = grass_region
    
if __name__ == "__main__":
    cmd = gtask.cmdstring_to_tuple(sys.argv[1])
    if not cmd[0] or cmd[0] == 'd.mon':
        sys.exit(0)
    path = os.path.dirname(os.path.abspath(__file__))
    mon = os.path.split(path)[-1]
    
    width, height = read_env_file(os.path.join(path, 'env'))
    if mon.startswith('wx'):
        mapfile = tempfile.NamedTemporaryFile(dir=path).name
        if cmd[0] in ('d.barscale', 'd.legend', 'd.northarrow'):
            mapfile += '.png'
        else:
            mapfile += '.ppm'
    else:
        mapfile = None
        adjust_region(width, height)
        

    render(cmd, mapfile)
    update_cmd_file(os.path.join(path, 'cmd'), cmd, mapfile)
        
    sys.exit(0)
