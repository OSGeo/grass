# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:31:47 2013

@author: pietro
"""

from __future__ import print_function
import re


from read import GETTYPE, element2dict, DOC


class Parameter(object):

    def __init__(self, xparameter=None, diz=None):
        self._value = None
        diz = element2dict(xparameter) if xparameter is not None else diz
        if diz is None:
            raise TypeError('Xparameter or diz are required')
        self.name = diz['name']
        self.required = True if diz['required'] == 'yes' else False
        self.multiple = True if diz['multiple'] == 'yes' else False
        # check the type
        if diz['type'] in GETTYPE:
            self.type = GETTYPE[diz['type']]
            self.typedesc = diz['type']
        else:
            raise TypeError('New type: %s, ignored' % diz['type'])

        self.description = diz.get('description', None)
        self.keydesc, self.keydescvalues = diz.get('keydesc', (None, None))

        #
        # values
        #
        if 'values' in diz:
            try:
                # Check for integer ranges: "3-30"
                isrange = re.match("(?P<min>\d+)-(?P<max>\d+)",
                                   diz['values'][0])
                if isrange:
                    range_min, range_max = isrange.groups()
                    self.values = range(int(range_min), int(range_max) + 1)
                    self.isrange = diz['values'][0]
                # Check for float ranges: "0.0-1.0"
                if not isrange:
                    isrange = re.match("(?P<min>\d+.\d+)-(?P<max>\d+.\d+)",
                                       diz['values'][0])
                    if isrange:
                        # We are not able to create range values from
                        # floating point ranges
                        self.isrange = diz['values'][0]
                # No range was found
                if not isrange:
                    self.values = [self.type(i) for i in diz['values']]
                    self.isrange = False
            except TypeError:
                self.values = [self.type(i) for i in diz['values']]
                self.isrange = False

        #
        # default
        #
        if 'default' in diz:
            if self.multiple or self.keydescvalues:
                self.default = [self.type(v)
                                for v in diz['default'].split(',')]
            else:
                self.default = self.type(diz['default'])
            self._value = self.default
        else:
            self.default = None

        self.guisection = diz.get('guisection', None)

        #
        # gisprompt
        #
        if 'gisprompt' in diz:
            self.typedesc = diz['gisprompt']['prompt']
            self.input = False if diz['gisprompt']['age'] == 'new' else True
        else:
            self.input = True

    def _get_value(self):
        return self._value

    def _set_value(self, value):
        if value is None:
            self._value = value
        elif isinstance(value, list) or isinstance(value, tuple):
            if self.multiple or self.keydescvalues:
                # check each value
                self._value = [self.type(val) for val in value]
            else:
                str_err = 'The Parameter <%s> does not accept multiple inputs'
                raise TypeError(str_err % self.name)
        elif self.typedesc == 'all':
            self._value = value
        elif isinstance(value, self.type):
            if hasattr(self, 'values'):
                if self.type is float:
                    if self.values[0] <= value <= self.values[-1]:
                        self._value = value
                    else:
                        err_str = 'The Parameter <%s>, must be: %g<=value<=%g'
                        raise ValueError(err_str % (self.name, self.values[0],
                                                    self.values[-1]))
                elif value in self.values:
                    self._value = value
                else:
                    raise ValueError('The Parameter <%s>, must be one of: %r' %
                                     (self.name, self.values))
            else:
                self._value = value
        elif self.type is str and isinstance(value, unicode):
            if hasattr(self, 'values'):
                if value in self.values:
                    self._value = value
                else:
                    raise ValueError('The Parameter <%s>, must be one of: %r' %
                                     (self.name, self.values))
            else:
                self._value = value
        else:
            str_err = 'The Parameter <%s>, require: %s, get: %s instead'
            raise TypeError(str_err % (self.name, self.typedesc, type(value)))

    # here the property function is used to transform value in an attribute
    # in this case we define which function must be use to get/set the value
    value = property(fget=_get_value, fset=_set_value)

    def get_bash(self):
        if isinstance(self._value, list) or isinstance(self._value, tuple):
            value = ','.join([str(v) for v in self._value])
        else:
            value = str(self._value)
        return """%s=%s""" % (self.name, value)

    def get_python(self):
        if not self.value:
            return ''
        return """%s=%r""" % (self.name, self._value)

    def __str__(self):
        return self.get_bash()

    def __repr__(self):
        str_repr = "Parameter <%s> (required:%s, type:%s, multiple:%s)"
        return str_repr % (self.name,
                           "yes" if self.required else "no",
                           self.type if self.type in (
                           'raster', 'vector') else self.typedesc,
                           "yes" if self.multiple else "no")

    # here we use property with a decorator, in this way we mask a method as
    # a class attribute
    @property
    def __doc__(self):
        """Return the docstring of the parameter

        {name}: {default}{required}{multi}{ptype}
            {description}{values}"","""
        if hasattr(self, 'values'):
            if self.isrange:
                vals = self.isrange
            else:
                vals = ', '.join([repr(val) for val in self.values])
        else:
            vals = False
        if self.keydescvalues:
            keydescvals = "\n    (%s)" % ', '.join(self.keydescvalues)
        return DOC['param'].format(name=self.name,
                default=repr(self.default) + ', ' if self.default else '',
                required='required, ' if self.required else 'optional, ',
                multi='multi' if self.multiple else '',
                ptype=self.typedesc, description=self.description,
                values='\n    Values: {0}'.format(vals)  if vals else '',
                keydescvalues= keydescvals if self.keydescvalues else '')
