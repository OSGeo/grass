# -*- coding: utf-8 -*-
"""
Created on Wed Aug 15 17:33:27 2012

@author: pietro
"""


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