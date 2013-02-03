# -*- coding: utf-8 -*-
"""
Created on Wed Aug 15 17:33:27 2012

@author: pietro
"""

from grass.script import warning


class GrassError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


class OpenError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


def must_be_open(method):
    def wrapper(self, *args, **kargs):
        if self.is_open():
            return method(self, *args, **kargs)
        else:
            warning(_("The map is close!"))
    return wrapper
