# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:31:47 2013

@author: pietro
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
import re

from grass.pygrass.modules.interface.docstring import docstring_property
from grass.pygrass.modules.interface.read import GETTYPE, element2dict, DOC


def _check_value(param, value):
    """Function to check the correctness of a value and
    return the checked value and the original.
    """
    must_val = 'The Parameter <%s>, must be one of the following values: %r'
    req = 'The Parameter <%s>, require: %s, get: %s instead: %r\n%s'
    string = (type(b''), type(u''))

    def raiseexcpet(exc, param, ptype, value):
        """Function to modifa the error message"""
        msg = req % (param.name, param.typedesc, ptype, value, str(exc))
        if isinstance(exc, ValueError):
            raise ValueError(msg)
        elif isinstance(exc, TypeError):
            raise TypeError(msg)
        else:
            exc.message = msg
            raise exc

    def check_string(value):
        """Function to check that a string parameter is already a string"""
        if param.type in string:
            if type(value) in (int, float):
                value = str(value)
            if type(value) not in string:
                msg = ("The Parameter <%s> require a string,"
                       " %s instead is provided: %r")
                raise ValueError(msg % (param.name, type(value), value))
        return value

    # return None if None
    if value is None:
        return param.default, param.default

    # find errors with multiple parmeters
    if isinstance(value, (list, tuple)):
        if param.keydescvalues:
            return (([value, ], value) if isinstance(value, tuple)
                    else (value, value))
        if param.multiple:
            # everything looks fine, so check each value
            try:
                return [param.type(check_string(val)) for val in value], value
            except Exception as exc:
                raiseexcpet(exc, param, param.type, value)
        else:
            msg = 'The Parameter <%s> does not accept multiple inputs'
            raise TypeError(msg % param.name)

    if param.keydescvalues:
        msg = 'The Parameter <%s> require multiple inputs in the form: %s'
        raise TypeError(msg % (param.name, param.keydescvalues))

    if param.typedesc == 'all':
        return value, value

    # check string before trying to convert value to the correct type
    check_string(value)
    # the value is a scalar
    try:
        newvalue = param.type(value)
    except Exception as exc:
        raiseexcpet(exc, param, type(value), value)

    # check values
    if hasattr(param, 'values'):
        if param.type in (float, int):
            # check for value in range
            if ((param.min is not None and newvalue < param.min) or
                    (param.max is not None and newvalue > param.max)):
                err_str = ('The Parameter <%s>, must be between: '
                           '%g<=value<=%g, %r is outside.')
                raise ValueError(err_str % (param.name, param.min,
                                            param.max, newvalue))
        # check if value is in the list of valid values
        if param.values is not None and newvalue not in param.values:
            raise ValueError(must_val % (param.name, param.values))
    return (([newvalue, ] if (param.multiple or param.keydescvalues)
             else newvalue), value)


# TODO add documentation
class Parameter(object):
    """The Parameter object store all information about a parameter of a
    GRASS GIS module. ::

        >>> param = Parameter(diz=dict(name='int_number', required='yes',
        ...                            multiple='no', type='integer',
        ...                            values=[2, 4, 6, 8]))
        >>> param.value = 2
        >>> param.value
        2
        >>> param.value = 3
        Traceback (most recent call last):
           ...
        ValueError: The Parameter <int_number>, must be one of the following values: [2, 4, 6, 8]

    ...
    """
    def __init__(self, xparameter=None, diz=None):
        self._value = None
        self._rawvalue = None
        self.min = None
        self.max = None
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
                # Check for integer ranges: "3-30" or float ranges: "0.0-1.0"
                isrange = re.match("(?P<min>-*\d+.*\d*)*-(?P<max>\d+.*\d*)*",
                                   diz['values'][0])
                if isrange:
                    mn, mx = isrange.groups()
                    self.min = None if mn is None else float(mn)
                    self.max = None if mx is None else float(mx)
                    self.values = None
                    self.isrange = diz['values'][0]
                # No range was found
                else:
                    self.values = [self.type(i) for i in diz['values']]
                    self.isrange = False
            except TypeError:
                self.values = [self.type(i) for i in diz['values']]
                self.isrange = False

        #
        # default
        #
        if 'default' in diz and diz['default']:
            if self.multiple or self.keydescvalues:
                self.default = [self.type(v)
                                for v in diz['default'].split(',')]
            else:
                self.default = self.type(diz['default'])
        else:
            self.default = None
        self._value, self._rawvalue = self.default, self.default
        self.guisection = diz.get('guisection', None)

        #
        # gisprompt
        #
        if 'gisprompt' in diz and diz['gisprompt']:
            self.typedesc = diz['gisprompt'].get('prompt', '')
            self.input = False if diz['gisprompt']['age'] == 'new' else True
        else:
            self.input = True

    def _get_value(self):
        return self._value

    def _set_value(self, value):
        self._value, self._rawvalue = _check_value(self, value)

    # here the property function is used to transform value in an attribute
    # in this case we define which function must be use to get/set the value
    value = property(fget=_get_value, fset=_set_value,
                     doc="Parameter value transformed and validated.")

    @property
    def rawvalue(self):
        """Parameter value as insert by user without transformation"""
        return self._rawvalue

    def get_bash(self):
        """Return the BASH representation of the parameter. ::

            >>> param = Parameter(diz=dict(name='int_number', required='yes',
            ...                            multiple='no', type='integer',
            ...                            values=[2, 4, 6, 8], default=8))
            >>> param.get_bash()
            'int_number=8'

        ..
        """
        sep = ','
        if isinstance(self.rawvalue, (list, tuple)):
            value = sep.join([sep.join([str(v) for v in val])
                              if isinstance(val, tuple) else str(val)
                              for val in self.rawvalue])
        else:
            value = str(self.rawvalue)
        return "%s=%s" % (self.name, value)

    def get_python(self):
        """Return a string with the Python representation of the parameter. ::

            >>> param = Parameter(diz=dict(name='int_number', required='yes',
            ...                            multiple='no', type='integer',
            ...                            values=[2, 4, 6, 8], default=8))
            >>> param.get_python()
            'int_number=8'

        ..
        """
        if self.value is None:
            return ''
        return """%s=%r""" % (self.name, self.value)

    def __str__(self):
        """Return the BASH representation of the GRASS module parameter."""
        return self.get_bash()

    def __repr__(self):
        """Return the python representation of the GRASS module parameter."""
        str_repr = "Parameter <%s> (required:%s, type:%s, multiple:%s)"
        mtype = ('raster', 'vector')  # map type
        return str_repr % (self.name,
                           "yes" if self.required else "no",
                           self.type if self.type in mtype else self.typedesc,
                           "yes" if self.multiple else "no")

    @docstring_property(__doc__)
    def __doc__(self):
        """Return the docstring of the parameter

        {name}: {default}{required}{multi}{ptype}
            {description}{values}"",

        ::

            >>> param = Parameter(diz=dict(name='int_number',
            ...                            description="Set an number",
            ...                            required='yes',
            ...                            multiple='no', type='integer',
            ...                            values=[2, 4, 6, 8], default=8))
            >>> print(param.__doc__)
            int_number: 8, required, integer
                Set an number
                Values: 2, 4, 6, 8
        ..
        """
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
