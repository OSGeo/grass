# -*- coding: utf-8 -*-
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
import fnmatch


from grass.script.core import get_commands
from grass.pygrass.modules.interface import Module

_CMDS = list(get_commands()[0])
_CMDS.sort()


class MetaModule(object):
    """Example how to use MetaModule

       >>> g = MetaModule('g')
       >>> g_list = g.list
       >>> g_list.name
       'g.list'
       >>> g_list.required
       ['type']
       >>> g_list.inputs.type = 'raster'
       >>> g_list.inputs.mapset = 'PERMANENT'
       >>> g_list.stdout_ = -1
       >>> g_list.run()
       Module('g.list')
       >>> g_list.outputs.stdout                         # doctest: +ELLIPSIS
       '...basin...elevation...'

       >>> r = MetaModule('r')
       >>> what = r.what
       >>> what.description
       'Queries raster maps on their category values and category labels.'
       >>> what.inputs.map = 'elevation'
       >>> what.inputs.coordinates = [640000,220500]          # doctest: +SKIP
       >>> what.run()                                         # doctest: +SKIP
       >>> v = MetaModule('v')
       >>> v.import                                      # doctest: +ELLIPSIS
       Traceback (most recent call last):
         File ".../doctest.py", line 1315, in __run
          compileflags, 1) in test.globs
         File "<doctest grass.pygrass.modules.shortcuts.MetaModule[16]>", line 1
          v.import
               ^
       SyntaxError: invalid syntax
       >>> v.import_
       Module('v.import')
    """
    def __init__(self, prefix, cls=None):
        self.prefix = prefix
        self.cls = cls if cls else Module

    def __dir__(self):
        return [mod[(len(self.prefix) + 1):].replace('.', '_')
                for mod in fnmatch.filter(_CMDS, "%s.*" % self.prefix)]

    def __getattr__(self, name):
        return self.cls('%s.%s' % (self.prefix,
                                   name.strip('_').replace('_', '.')))


# https://grass.osgeo.org/grass78/manuals/full_index.html
#[ d.* | db.* | g.* | i.* | m.* | ps.* | r.* | r3.* | t.* | v.* ]
#
#  d.*	display commands
#  db.*	database commands
#  g.*	general commands
#  i.*	imagery commands
#  m.*	miscellaneous commands
#  ps.*	postscript commands
#  r.*	raster commands
#  r3.*	3D raster commands
#  t.*	temporal commands
#  v.*	vector commands

display = MetaModule('d')
database = MetaModule('db')
general = MetaModule('g')
imagery = MetaModule('i')
miscellaneous = MetaModule('m')
postscript = MetaModule('ps')
raster = MetaModule('r')
raster3d = MetaModule('r3')
temporal = MetaModule('t')
vector = MetaModule('v')
