# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:37:02 2013

@author: pietro
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
from copy import deepcopy
try:
    from collections import OrderedDict
except ImportError:
    from grass.pygrass.orderdict import OrderedDict

from grass.pygrass.modules.interface.docstring import docstring_property


class TypeDict(OrderedDict):
    def __init__(self, dict_type, *args, **kargs):
        self._type = dict_type
        super(TypeDict, self).__init__(*args, **kargs)

    def __getattr__(self, key):
        if key in self:
            return self[key].value
        return OrderedDict.__getattr__(self, key)

    def __setattr__(self, key, value):
        if key in self:
            self[key].value = value
        else:
            OrderedDict.__setattr__(self, key, value)

    def __dir__(self):
        return self.keys()

    def __setitem__(self, key, value):
        if isinstance(value, self._type):
            super(TypeDict, self).__setitem__(key, value)
        else:
            str_err = 'The value: %r is not a %s instance.'
            raise TypeError(str_err % (value, self._type.__name__))

    @docstring_property(__doc__)
    def __doc__(self):
        return '\n'.join([self.__getitem__(obj).__doc__
                          for obj in self.__iter__()])

    def __call__(self):
        return [self.__getitem__(obj) for obj in self.__iter__()]

    def __deepcopy__(self, memo):
        obj = TypeDict(self._type)
        for k, v in self.items():
            obj[k] = deepcopy(v)
        return obj

    def used(self):
        key_dict = {}
        for key in self:
            if self.__getattr__(key):
                key_dict[key] = self.__getattr__(key)
        return key_dict