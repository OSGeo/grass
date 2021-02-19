# -*- coding: utf-8 -*-
"""
Created on Fri Jul  4 16:32:54 2014

@author: pietro
"""
from __future__ import print_function
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.modules.interface.parameter import Parameter, _check_value

GETTYPE = {
    'string': str,
    'integer': int,
    'float': float,
    'double': float,
    'file': str,
    'all': lambda x: x,
}

class TestCheckValueFunction(TestCase):

    def test_single_all(self):
        param = Parameter(diz=dict(name='int_number', required='yes',
                                   multiple='no', type='all'))
        value = 1
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = 1.2
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = "elev"
        self.assertTupleEqual((value, value), _check_value(param, value))

        # test errors
        with self.assertRaises(TypeError):
            _check_value(param, (1, 2))

    def test_single_float_double(self):
        for ptype in ('float', 'double'):
            param = Parameter(diz=dict(name='int_number', required='yes',
                                       multiple='no', type=ptype))
            value = 1
            self.assertTupleEqual((float(value), value), _check_value(param, value))
            value = 1.2
            self.assertTupleEqual((value, value), _check_value(param, value))
            value = "1"
            self.assertTupleEqual((float(value), value), _check_value(param, value))
            value = "1.35"
            self.assertTupleEqual((float(value), value), _check_value(param, value))

            # test errors
            with self.assertRaises(ValueError):
                _check_value(param, "elev")
            with self.assertRaises(TypeError):
                _check_value(param, (1., 2.))

    def test_multiple_float_double(self):
        for ptype in ('float', 'double'):
            param = Parameter(diz=dict(name='number', required='yes',
                                       multiple='yes', type=ptype))
            value = (1.4, 2.3)
            self.assertTupleEqual((list(value), value),
                                  _check_value(param, value))
            value = (1, 2)
            self.assertTupleEqual(([float(v) for v in value], value),
                                  _check_value(param, value))
            value = ("1", "2")
            self.assertTupleEqual(([float(v) for v in value], value),
                                  _check_value(param, value))
            value = ("1.4", "2.3")
            self.assertTupleEqual(([float(v) for v in value], value),
                                  _check_value(param, value))
            value = 1.
            self.assertTupleEqual(([value, ], value),
                                  _check_value(param, value))
            value = 1
            self.assertTupleEqual(([value, ], value),
                                  _check_value(param, value))

            # test errors
            with self.assertRaises(ValueError):
                _check_value(param, "elev")
            with self.assertRaises(ValueError):
                _check_value(param, ("elev", "slope", "aspect"))

    def test_range_float_double(self):
        for ptype in ('float', 'double'):
            param = Parameter(diz=dict(name='int_number', required='yes',
                                       multiple='no', type=ptype,
                                       values=["0.0-2.5", ]))
            value = 1
            self.assertTupleEqual((float(value), value), _check_value(param, value))
            value = 1.2
            self.assertTupleEqual((value, value), _check_value(param, value))
            value = "0"
            self.assertTupleEqual((float(value), value), _check_value(param, value))
            value = "2.5"
            self.assertTupleEqual((float(value), value), _check_value(param, value))

            # test errors
            with self.assertRaises(ValueError):
                _check_value(param, "elev")
            with self.assertRaises(TypeError):
                _check_value(param, (1., 2.))
            with self.assertRaises(ValueError):
                _check_value(param, -1.)
            with self.assertRaises(ValueError):
                _check_value(param, 2.6)

    def test_single_integer(self):
        param = Parameter(diz=dict(name='int_number', required='yes',
                                   multiple='no', type='integer'))
        value = 1
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = 1.2
        self.assertTupleEqual((int(value), value), _check_value(param, value))
        value = "1"
        self.assertTupleEqual((int(value), value), _check_value(param, value))

        # test errors
        with self.assertRaises(ValueError):
            _check_value(param, "1.")
        with self.assertRaises(ValueError):
            _check_value(param, "elev")
        with self.assertRaises(TypeError):
            _check_value(param, (1, 2))

    def test_multiple_integer(self):
        param = Parameter(diz=dict(name='int_number', required='yes',
                                   multiple='yes', type='integer'))
        value = (1, 2)
        #import ipdb; ipdb.set_trace()
        self.assertTupleEqual((list(value), value), _check_value(param, value))
        value = (1.2, 2.3)
        self.assertTupleEqual(([int(v) for v in value], value),
                              _check_value(param, value))
        value = ("1", "2")
        self.assertTupleEqual(([int(v) for v in value], value),
                              _check_value(param, value))
        value = 1
        self.assertTupleEqual(([1, ], value), _check_value(param, value))
        value = 1.2
        self.assertTupleEqual(([int(value), ], value),
                              _check_value(param, value))
        value = "1"
        self.assertTupleEqual(([int(value), ], value),
                              _check_value(param, value))

        # test errors
        with self.assertRaises(ValueError):
            _check_value(param, "elev")
        with self.assertRaises(ValueError):
            _check_value(param, ("elev", "slope", "aspect"))

    def test_keydescvalues(self):
        for ptype in ('integer', 'float'):
            param = Parameter(diz=dict(name='int_number', required='yes',
                                       multiple='yes',
                                       keydesc=('range', '(min, max)'),
                                       type='integer'))
            value = (1, 2)
            self.assertTupleEqual(([value, ], value),
                                  _check_value(param, value))
            value = [(1, 2), (2, 3)]
            self.assertTupleEqual((value, value), _check_value(param, value))

            with self.assertRaises(TypeError):
                _check_value(param, 1)

    def test_range_integer(self):
        param = Parameter(diz=dict(name='int_number', required='yes',
                                   multiple='no', type='integer',
                                   values=["0-10", ]))
        value = 1
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = 0
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = 10
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = 1.2
        self.assertTupleEqual((int(value), value), _check_value(param, value))
        value = "1"
        self.assertTupleEqual((int(value), value), _check_value(param, value))

        # test errors
        with self.assertRaises(ValueError):
            _check_value(param, "1.")
        with self.assertRaises(ValueError):
            _check_value(param, "elev")
        with self.assertRaises(TypeError):
            _check_value(param, (1, 2))
        with self.assertRaises(ValueError):
            _check_value(param, -1)
        with self.assertRaises(ValueError):
            _check_value(param, 11)

    def test_choice_integer(self):
        param = Parameter(diz=dict(name='int_number', required='yes',
                                   multiple='no', type='integer',
                                   values=[2, 4, 6, 8]))
        value = 4
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = 2
        self.assertTupleEqual((int(value), value), _check_value(param, value))
        value = "8"
        self.assertTupleEqual((int(value), value), _check_value(param, value))

        # test errors
        with self.assertRaises(ValueError):
            _check_value(param, "2.")
        with self.assertRaises(ValueError):
            _check_value(param, "elev")
        with self.assertRaises(TypeError):
            _check_value(param, (1, 2))
        with self.assertRaises(ValueError):
            _check_value(param, 3)

    def test_single_string_file(self):
        for ptype in ('string', 'file'):
            param = Parameter(diz=dict(name='name', required='yes',
                                       multiple='no', type=ptype))
            value = u'elev'
            self.assertTupleEqual((value, value), _check_value(param, value))
            value = 10
            self.assertTupleEqual((str(value), value),
                                  _check_value(param, value))
            value = 12.5
            self.assertTupleEqual((str(value), value),
                                  _check_value(param, value))

            # test errors
            with self.assertRaises(TypeError):
                _check_value(param, ('abc', 'def'))

    def test_multiple_strings(self):
        param = Parameter(diz=dict(name='rastnames', required='yes',
                                   multiple='yes', type='string'))
        value = ['elev', 'slope', 'aspect']
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = ('elev', 'slope', 'aspect')
        self.assertTupleEqual((list(value), value), _check_value(param, value))
        value = ['1.3', '2.3', '4.5']
        self.assertTupleEqual((value, value), _check_value(param, value))
        value = [1.3, 2.3, 4.5]
        self.assertTupleEqual(([str(v) for v in value], value),
                              _check_value(param, value))
        value = (1, 2, 3)
        self.assertTupleEqual(([str(v) for v in value], value),
                              _check_value(param, value))
        value = 'elev'
        self.assertTupleEqual(([value, ], value), _check_value(param, value))

        # test errors
        with self.assertRaises(ValueError):
            _check_value(param, ({}, {}, {}))

    def test_choice_string(self):
        values = ["elev", "asp", "slp"]
        param = Parameter(diz=dict(name='rastname', required='yes',
                                   multiple='no', type='string',
                                   values=values))
        value = "asp"
        self.assertTupleEqual((value, value), _check_value(param, value))

        # test errors
        with self.assertRaises(ValueError):
            _check_value(param, "2")
        with self.assertRaises(ValueError):
            _check_value(param, "2.")
        with self.assertRaises(TypeError):
            _check_value(param, (1, 2))
        with self.assertRaises(ValueError):
            _check_value(param, "elevation")


class TestParameterGetBash(TestCase):
    def test_single_float_double(self):
        for ptype in ('float', 'double'):
            param = Parameter(diz=dict(name='number', required='yes',
                                       multiple='no', type=ptype))
            # set private attributes to skip the check function
            param._value = 1.0
            param._rawvalue = 1.0
            self.assertEqual("number=1.0", param.get_bash())
            param._value = 1.0
            param._rawvalue = "1."
            self.assertEqual("number=1.", param.get_bash())

    def test_multiple_float_double(self):
        for ptype in ('float', 'double'):
            param = Parameter(diz=dict(name='number', required='yes',
                                       multiple='yes', type=ptype))
            # set private attributes to skip the check function
            param._value = [1.0, ]
            param._rawvalue = 1.0
            self.assertEqual("number=1.0", param.get_bash())
            param._value = [1.0, ]
            param._rawvalue = "1."
            self.assertEqual("number=1.", param.get_bash())
            param._value = [1.0, 2.0, 3.0]
            param._rawvalue = [1.0, 2.0, 3.0]
            self.assertEqual("number=1.0,2.0,3.0", param.get_bash())
            param._value = [1.0, 2.0, 3.0]
            param._rawvalue = ["1.", "2.", "3."]
            self.assertEqual("number=1.,2.,3.", param.get_bash())

    def test_single_string(self):
        param = Parameter(diz=dict(name='rast', required='yes',
                                   multiple='no', type='string'))
        # set private attributes to skip the check function
        param._value = 'elev'
        param._rawvalue = 'elev'
        self.assertEqual("rast=elev", param.get_bash())

    def test_multiple_strings(self):
        param = Parameter(diz=dict(name='rast', required='yes',
                                   multiple='yes', type='string'))
        # set private attributes to skip the check function
        param._value = ['elev', 'asp', 'slp']
        param._rawvalue = ['elev', 'asp', 'slp']
        self.assertEqual("rast=elev,asp,slp", param.get_bash())
        param._value = ['elev', ]
        param._rawvalue = 'elev'
        self.assertEqual("rast=elev", param.get_bash())

    def test_keydescvalues(self):
        param = Parameter(diz=dict(name='range', required='yes',
                                   multiple='yes',
                                   keydesc=('range', '(min, max)'),
                                   type='integer'))
        # set private attributes to skip the check function
        param._value = [(1., 2.), ]
        param._rawvalue = (1., 2.)
        self.assertEqual("range=1.0,2.0", param.get_bash())
        param._value = [(1., 2.), (3., 4.)]
        param._rawvalue = [(1., 2.), (3., 4.)]
        self.assertEqual("range=1.0,2.0,3.0,4.0", param.get_bash())
        param._value = [(1., 2.), (3., 4.)]
        param._rawvalue = [('1.0', '2.00'), ('3.000', '4.0000')]
        self.assertEqual("range=1.0,2.00,3.000,4.0000", param.get_bash())

        with self.assertRaises(TypeError):
            _check_value(param, 1)

if __name__ == '__main__':
    test()
