#!/usr/bin/env python
import os
import sys

from grass.script import core as grass
from grass.script import task as gtask

cmd, dcmd = gtask.cmdstring_to_tuple(sys.argv[1])
if not cmd or cmd == 'd.mon':
    sys.exit(0)

path = os.path.dirname(os.path.abspath(__file__))
cmd_file = os.path.join(path, 'cmd')
env_file = os.path.join(path, 'env')

# read environment variables from file
fd = open(env_file, 'r')
if fd is None:
    grass.fatal("Unable to open file '%s'" % env_file)
lines = fd.readlines()
for l in lines:
    if l.startswith('#'):
         continue
    k, v = l.rstrip('\n').split('#', 1)[0].strip().split('=', 1)
    os.environ[k] = v
fd.close()

# run display command
try:
    grass.run_command(cmd, **dcmd)
except Exception as e:
    sys.exit("ERROR: %s" % e)

# update cmd file
ignoredCmd = ('d.colorlist', 'd.font', 'd.fontlist',
              'd.frame', 'd.info', 'd.mon', 'd.out.file',
              'd.redraw', 'd.to.rast', 'd.what.rast',
              'd.what.vect', 'd.where')
if cmd not in ignoredCmd:
    mode = 'w' if cmd == 'd.erase' else 'a'
    # update cmd file
    fd = open(cmd_file, mode)
    if fd is None:
        grass.fatal("Unable to open file '%s'" % cmd_file)
    if mode == 'a':
        fd.write(sys.argv[1])
        fd.write('\n')
    else:
         fd.write('')
    fd.close()

sys.exit(0)
