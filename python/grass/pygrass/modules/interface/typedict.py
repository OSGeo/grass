"""
Created on Tue Apr  2 18:37:02 2013

@author: pietro
"""

from collections import OrderedDict
from copy import deepcopy

from grass.pygrass.modules.interface.docstring import docstring_property


class TypeDict(OrderedDict):
    def __init__(self, dict_type, *args, **kargs):
        self._type = dict_type
        super().__init__(*args, **kargs)

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
        if not isinstance(value, self._type):
            str_err = "The value: %r is not a %s instance."
            raise TypeError(str_err % (value, self._type.__name__))
        super().__setitem__(key, value)

    @docstring_property(__doc__)
    def __doc__(self):
        return "\n".join([self.__getitem__(obj).__doc__ for obj in self.__iter__()])

    def __call__(self):
        return [self.__getitem__(obj) for obj in self.__iter__()]

    def __deepcopy__(self, memo):
        obj = TypeDict(self._type)
        for k, v in self.items():
            obj[k] = deepcopy(v)
        return obj

    def __reduce__(self):
        inst_dict = vars(self).copy()
        for k in vars(TypeDict(self._type)):
            inst_dict.pop(k, None)
        return (
            self.__class__,
            (self._type,),
            inst_dict or None,
            None,
            iter(self.items()),
        )

    def used(self):
        return {key: getattr(self, key) for key in self if getattr(self, key)}
