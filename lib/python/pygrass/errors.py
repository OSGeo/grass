# -*- coding: utf-8 -*-
from functools import wraps

from grass.exceptions import (FlagError, ParameterError, DBError,
                              GrassError, OpenError)

from grass.pygrass.messages import get_msgr


def must_be_open(method):

    @wraps(method)
    def wrapper(self, *args, **kargs):
        if self.is_open():
            return method(self, *args, **kargs)
        else:
            get_msgr().warning(_("The map is close!"))
    return wrapper
