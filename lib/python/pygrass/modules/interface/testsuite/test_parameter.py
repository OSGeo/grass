# -*- coding: utf-8 -*-
"""
Created on Fri Jul  4 16:32:54 2014

@author: pietro
"""
from __future__ import print_function
import unittest

from grass.pygrass.modules.interface.parameter import Parameter, _check_value

GETTYPE = {
    'string': str,
    'integer': int,
    'float': float,
    'double': float,
    'file': str,
    'all': lambda x: x,
}

class TestCheckValueFunction(unittest.TestCase):

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

            # test errors
            with self.assertRaises(TypeError):
                _check_value(param, 1.)
            with self.assertRaises(TypeError):
                _check_value(param, 1)
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
        self.assertTupleEqual((list(value), value), _check_value(param, value))
        value = (1.2, 2.3)
        self.assertTupleEqual(([int(v) for v in value], value),
                              _check_value(param, value))
        value = ("1", "2")
        self.assertTupleEqual(([int(v) for v in value], value),
                              _check_value(param, value))

        # test errors
        with self.assertRaises(TypeError):
            _check_value(param, 1)
        with self.assertRaises(TypeError):
            _check_value(param, 1.0)
        with self.assertRaises(TypeError):
            _check_value(param, "1.")
        with self.assertRaises(TypeError):
            _check_value(param, "elev")
        with self.assertRaises(ValueError):
            _check_value(param, ("elev", "slope", "aspect"))

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
        #import ipdb; ipdb.set_trace()
        with self.assertRaises(ValueError):
            _check_value(param, 3)

    def test_single_string_file(self):
        for ptype in ('string', 'file'):
            param = Parameter(diz=dict(name='name', required='yes',
                                       multiple='no', type=ptype))
            value = u'elev'
            self.assertTupleEqual((value, value), _check_value(param, value))

            # test errors
            with self.assertRaises(ValueError):
                _check_value(param, 1)
            with self.assertRaises(ValueError):
                _check_value(param, 1.0)
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

        # test errors
        with self.assertRaises(ValueError):
            _check_value(param, (1, 2, 3))

    def test_choice_string(self):
        values = ["elev", "asp", "slp"]
        param = Parameter(diz=dict(name='rastname', required='yes',
                                   multiple='no', type='string',
                                   values=values))
        value="asp"
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


if __name__ == '__main__':
    unittest.main()
