# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:37:02 2013

@author: pietro
"""
from copy import deepcopy
try:
    from collections import OrderedDict
except ImportError:
    from grass.pygrass.orderdict import OrderedDict


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
            cl = repr(self._type).translate(None, "'<> ").split('.')
            str_err = 'The value: %r is not a %s object'
            raise TypeError(str_err % (value, cl[-1].title()))

    @property
    def __doc__(self):
        return '\n'.join([self.__getitem__(obj).__doc__
                          for obj in self.__iter__()])

    def __call__(self):
        return [self.__getitem__(obj) for obj in self.__iter__()]

    def __deepcopy__(self, memo):
        obj = TypeDict(self._type)
        for k, v in self.iteritems():
            obj[k] = deepcopy(v)
        return obj

    def used(self):
        key_dict = {}
        for key in self:
            if self.__getattr__(key):
                key_dict[key] = self.__getattr__(key)
        return key_dict