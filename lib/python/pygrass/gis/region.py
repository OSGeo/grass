# -*- coding: utf-8 -*-
"""
Created on Fri May 25 12:57:10 2012

@author: Pietro Zambelli
"""
import ctypes
import grass.lib.gis as libgis
import grass.script as grass

from pygrass.errors import GrassError


class Region(object):
    def __init__(self, default=False):
        """::

            >>> default = Region(default=True)
            >>> current = Region()
            >>> default == current
            True
            >>> current.cols
            1500
            >>> current.ewres
            10.0
            >>> current.cols = 3000
            >>> current.ewres
            5.0
            >>> current.ewres = 20.0
            >>> current.cols
            750
            >>> current.set_current()
            >>> default == current
            False
            >>> current.set_default()
            >>> default = Region(default=True)
            >>> default == current
            True
            >>> default
            Region(n=228500, s=215000, e=645000, w=630000, nsres=10, ewres=20)
            >>> current
            Region(n=228500, s=215000, e=645000, w=630000, nsres=10, ewres=20)
            >>> default.ewres = 10.
            >>> default.set_default()
        """
        self.c_region = ctypes.pointer(libgis.Cell_head())
        if default:
            self.get_default()
        else:
            self.get_current()

    def _set_param(self, key, value):
        grass.run_command('g.region', **{key: value})

    #----------LIMITS----------
    def _get_n(self):
        return self.c_region.contents.north

    def _set_n(self, value):
        self.c_region.contents.north = value

    north = property(fget=_get_n, fset=_set_n)

    def _get_s(self):
        return self.c_region.contents.south

    def _set_s(self, value):
        self.c_region.contents.south = value

    south = property(fget=_get_s, fset=_set_s)

    def _get_e(self):
        return self.c_region.contents.east

    def _set_e(self, value):
        self.c_region.contents.east = value

    east = property(fget=_get_e, fset=_set_e)

    def _get_w(self):
        return self.c_region.contents.west

    def _set_w(self, value):
        self.c_region.contents.west = value

    west = property(fget=_get_w, fset=_set_w)

    def _get_t(self):
        return self.c_region.contents.top

    def _set_t(self, value):
        self.c_region.contents.top = value

    top = property(fget=_get_t, fset=_set_t)

    def _get_b(self):
        return self.c_region.contents.bottom

    def _set_b(self, value):
        self.c_region.contents.bottom = value

    bottom = property(fget=_get_b, fset=_set_b)

    #----------RESOLUTION----------
    def _get_rows(self):
        return self.c_region.contents.rows

    def _set_rows(self, value):
        self.c_region.contents.rows = value
        self.adjust(rows=True)

    rows = property(fget=_get_rows, fset=_set_rows)

    def _get_cols(self):
        return self.c_region.contents.cols

    def _set_cols(self, value):
        self.c_region.contents.cols = value
        self.adjust(cols=True)

    cols = property(fget=_get_cols, fset=_set_cols)

    def _get_nsres(self):
        return self.c_region.contents.ns_res

    def _set_nsres(self, value):
        self.c_region.contents.ns_res = value
        self.adjust()

    nsres = property(fget=_get_nsres, fset=_set_nsres)

    def _get_ewres(self):
        return self.c_region.contents.ew_res

    def _set_ewres(self, value):
        self.c_region.contents.ew_res = value
        self.adjust()

    ewres = property(fget=_get_ewres, fset=_set_ewres)

    def _get_tbres(self):
        return self.c_region.contents.tb_res

    def _set_tbres(self, value):
        self.c_region.contents.tb_res = value
        self.adjust()

    tbres = property(fget=_get_tbres, fset=_set_tbres)

    @property
    def zone(self):
        return self.c_region.contents.zone

    @property
    def proj(self):
        return self.c_region.contents.proj

    #----------MAGIC METHODS----------
    def __repr__(self):
        return 'Region(n=%g, s=%g, e=%g, w=%g, nsres=%g, ewres=%g)' % (
               self.north, self.south, self.east, self.west,
               self.nsres, self.ewres)

    def __unicode__(self):
        return grass.pipe_command("g.region", flags="p").communicate()[0]

    def __str__(self):
        return self.__unicode__()

    def __eq__(self, reg):
        attrs = ['north', 'south', 'west', 'east', 'top', 'bottom',
                 'nsres', 'ewres', 'tbres']
        for attr in attrs:
            if getattr(self, attr) != getattr(reg, attr):
                return False
        return True

    def iteritems(self):
        return [('projection', self.proj),
                ('zone', self.zone),
                ('north', self.north),
                ('south', self.south),
                ('west', self.west),
                ('east', self.east),
                ('top', self.top),
                ('bottom', self.bottom),
                ('nsres', self.nsres),
                ('ewres', self.ewres),
                ('tbres', self.tbres),
                ('rows', self.rows),
                ('cols', self.cols),
                ('cells', self.rows * self.cols)]

    #----------METHODS----------
    def zoom(self, raster_name):
        """Shrink region until it meets non-NULL data from this raster map:"""
        self._set_param('zoom', str(raster_name))
        self.get_current()

    def align(self, raster_name):
        """Adjust region cells to cleanly align with this raster map"""
        self._set_param('align', str(raster_name))
        self.get_current()

    def adjust(self, rows=False, cols=False):
        """Adjust rows and cols number according with the nsres and ewres
        resolutions. If rows or cols parameters are True, the adjust method
        update nsres and ewres according with the rows and cols numbers.
        """
        libgis.G_adjust_Cell_head(self.c_region, bool(rows), bool(cols))

    def get_current(self):
        libgis.G_get_set_window(self.c_region)

    def set_current(self):
        libgis.G_set_window(self.c_region)

    def get_default(self):
        libgis.G_get_window(self.c_region)

    def set_default(self):
        self.adjust()
        if libgis.G_put_window(self.c_region) < 0:
            raise GrassError("Cannot change region (WIND file).")