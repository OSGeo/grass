# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:49:11 2013

@author: pietro
"""
from __future__ import print_function
import fnmatch


from grass.script.core import get_commands
from grass.pygrass.modules.interface.module import Module


_CMDS = list(get_commands()[0])
_CMDS.sort()


class MetaModule(object):
    """Example how to use MetaModule

       >>> g = MetaModule('g')
       >>> g_mlist = g.mlist
       >>> g_mlist.name
       'g.mlist'
       >>> g_mlist.required
       [Parameter <type> (required:yes, type:string, multiple:yes)]
       >>> g_mlist.inputs.type = 'rast'
       >>> g_mlist.stdout_ = -1
       >>> g_mlist.run()
       >>> g_mlist.outputs.stdout                         # doctest: +ELLIPSIS
       'basins...soils...'
       >>> r = MetaModule('r')
       >>> what = r.what
       >>> what.description
       'Queries raster maps on their category values and category labels.'
       >>> what.inputs.map = 'elevation'
       >>> what.inputs.coordinates = [640000,220500]          # doctest: +SKIP
       >>> what.run()                                         # doctest: +SKIP
    """
    def __init__(self, prefix, cls=None):
        self.prefix = prefix
        self.cls = cls if cls else Module

    def __dir__(self):
        return [mod[(len(self.prefix) + 1):].replace('.', '_')
                for mod in fnmatch.filter(_CMDS, "%s.*" % self.prefix)]

    def __getattr__(self, name):
        return self.class_('%s.%s' % (self.prefix, name.replace('_', '.')))


# http://grass.osgeo.org/grass70/manuals/html70_user/full_index.html
#[ d.* | db.* | g.* | i.* | m.* | ps.* | r.* | r3.* | t.* | v.* ]
#
#  d.*	display commands
#  db.*	database commands
#  g.*	general commands
#  i.*	imagery commands
#  m.*	miscellaneous commands
#  ps.*	postscript commands
#  r.*	raster commands
#  r3.*	raster3D commands
#  t.*	temporal commands
#  v.*	vector commands

display = MetaModule('d')
database = MetaModule('db')
general = MetaModule('g')
imagery = MetaModule('i')
miscellaneous = MetaModule('m')
postscript = MetaModule('ps')
raster = MetaModule('r')
raster3D = MetaModule('r3')
temporal = MetaModule('t')
vector = MetaModule('v')
