# -*- coding: utf-8 -*-
from functools import wraps

from grass.exceptions import (FlagError, ParameterError, DBError,
                              GrassError, OpenError)

from grass.pygrass.messages import get_msgr
import grass.lib.gis as libgis


def must_be_open(method):

    @wraps(method)
    def wrapper(self, *args, **kargs):
        if self.is_open():
            return method(self, *args, **kargs)
        else:
            msgr = get_msgr()
            msgr.warning(_("The map is close!"))
    return wrapper


def mapinfo_must_be_set(method):

    @wraps(method)
    def wrapper(self, *args, **kargs):
        if self.c_mapinfo:
            return method(self, *args, **kargs)
        else:
            raise GrassError(_("The self.c_mapinfo pointer must be "
                                 "correctly initiated"))
    return wrapper

def must_be_in_current_mapset(method):

    @wraps(method)
    def wrapper(self, *args, **kargs):
        if self.mapset == libgis.G_mapset().decode():
            return method(self, *args, **kargs)
        else:
            raise GrassError(_("Map <{}> not found in current mapset").format(
                self.name))

    return wrapper
