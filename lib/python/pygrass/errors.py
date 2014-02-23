# -*- coding: utf-8 -*-
"""
Created on Wed Aug 15 17:33:27 2012

@author: pietro
"""
from functools import wraps
from grass.pygrass.messages import get_msgr


class AbstractError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


class ParameterError(Exception):
    pass


class FlagError(Exception):
    pass


class DBError(AbstractError):
    pass


class GrassError(AbstractError):
    pass


class OpenError(AbstractError):
    pass


def must_be_open(method):

    @wraps(method)
    def wrapper(self, *args, **kargs):
        if self.is_open():
            return method(self, *args, **kargs)
        else:
            get_msgr().warning(_("The map is close!"))
    return wrapper
