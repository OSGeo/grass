# -*- coding: utf-8 -*-

"""
Tests checkers functions

@brief Test of GRASS Python testing framework checkers

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras
"""


from grass.script.utils import parse_key_val, try_remove

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.checkers import (
    values_equal, text_to_keyvalue,
    keyvalue_equals, proj_info_equals, proj_units_equals,
    file_md5, text_file_md5)



class TestValuesEqual(TestCase):

    def test_floats(self):
        self.assertTrue(values_equal(5.0, 5.0))
        self.assertTrue(values_equal(5.1, 5.19, precision=0.1))
        self.assertTrue(values_equal(5.00005, 5.000059, precision=0.00001))
        self.assertFalse(values_equal(5.125, 5.280))
        self.assertFalse(values_equal(5.00005, 5.00006, precision=0.00001))
        self.assertFalse(values_equal(2.5, 15.5, precision=5))

    def test_ints(self):
        self.assertTrue(values_equal(5, 5, precision=0.01))
        self.assertFalse(values_equal(5, 6, precision=0.01))
        self.assertTrue(values_equal(5, 8, precision=3))
        self.assertFalse(values_equal(3600, 3623, precision=20))
        self.assertTrue(values_equal(5, 5))
        self.assertFalse(values_equal(5, 6))

    def test_floats_and_ints(self):
        self.assertTrue(values_equal(5.1, 5, precision=0.2))
        self.assertFalse(values_equal(5.1, 5, precision=0.01))

    def test_strings(self):
        self.assertTrue(values_equal('hello', 'hello'))
        self.assertFalse(values_equal('Hello', 'hello'))

    def test_lists(self):
        self.assertTrue(values_equal([1, 2, 3], [1, 2, 3]))
        self.assertTrue(values_equal([1.1, 2.0, 3.9],
                                     [1.1, 1.95, 4.0],
                                     precision=0.2))
        self.assertFalse(values_equal([1, 2, 3, 4, 5],
                                      [1, 22, 3, 4, 5],
                                      precision=1))

    def test_mixed_lists(self):
        self.assertTrue(values_equal([1, 'abc', 8], [1, 'abc', 8.2],
                                     precision=0.5))

    def test_recursive_lists(self):
        self.assertTrue(values_equal([1, 'abc', [5, 9.6, 9.0]],
                                     [1, 'abc', [4.9, 9.2, 9.3]],
                                     precision=0.5))

KEYVAL_TEXT = '''s: Hello
str: Hello world!
f: 1.0
l: 1,2,3,4,5
mixed: hello,8,-25,world!,4-1,5:2,0.1,-9.6
'''

# file location/PERMANENT/PROJ_INFO
PROJ_INFO_TEXT_1 = """name: Lambert Conformal Conic
proj: lcc
datum: nad83
a: 6378137.0
es: 0.006694380022900787
lat_1: 36.16666666666666
lat_2: 34.33333333333334
lat_0: 33.75
lon_0: -79
x_0: 609601.22
y_0: 0
no_defs: defined
"""

# file location/PERMANENT/PROJ_UNITS
PROJ_UNITS_TEXT_1 = """unit: Meter
units: Meters
meters: 1
"""

PROJ_INFO_TEXT_2 = """name: Lambert Conformal Conic
proj: lcc
datum: nad83
a: 6378137.0000000002
es: 0.006694380022900787
lat_1:  36.166666667
lat_2: 34.333333333
lat_0:   33.75
lon_0:   -79
x_0: 609601.22
y_0: 0
no_defs: defined
"""

PROJ_UNITS_TEXT_2 = """unit: Metre
units: Meters
meters: 1
"""
# what about keys and lower/upper case letters


class TestTextToKeyValue(TestCase):
    def test_conversion(self):
        keyvals = text_to_keyvalue(KEYVAL_TEXT, sep=':', val_sep=',')
        expected = {'s': 'Hello',
                    'str': 'Hello world!',
                    'f': 1.0,
                    'l': [1, 2, 3, 4, 5],
                    'mixed': ['hello', 8, -25, 'world!',
                              '4-1', '5:2', 0.1, -9.6]}
        self.assertDictEqual(expected, keyvals)

    def test_single_values(self):
        keyvals = text_to_keyvalue("a: 1.5", sep=':')
        self.assertDictEqual({'a': 1.5}, keyvals)
        keyvals = text_to_keyvalue("abc=1", sep='=')
        self.assertDictEqual({'abc': 1}, keyvals)
        keyvals = text_to_keyvalue("abc=hello", sep='=')
        self.assertDictEqual({'abc': 'hello'}, keyvals)

    def test_strip(self):
        keyvals = text_to_keyvalue("a:   2.8  ", sep=':')
        self.assertDictEqual({'a': 2.8}, keyvals)
        keyvals = text_to_keyvalue("a:  2  ; 2.8 ; ab cd ",
                                   sep=':', val_sep=';')
        self.assertDictEqual({'a': [2, 2.8, 'ab cd']}, keyvals)
        keyvals = text_to_keyvalue("a  :  2  ; 2.8", sep=':', val_sep=';')
        self.assertDictEqual({'a': [2, 2.8]}, keyvals)
        keyvals = text_to_keyvalue("a  : \t 2  ;\t2.8", sep=':', val_sep=';')
        self.assertDictEqual({'a': [2, 2.8]}, keyvals)

    def test_empty_list_item(self):
        keyvals = text_to_keyvalue("a: 1, ,5,,", sep=':', val_sep=',')
        self.assertDictEqual({'a': [1, '', 5, '', '']}, keyvals)

    def test_empty_value(self):
        keyvals = text_to_keyvalue("a: ", sep=':')
        self.assertDictEqual({'a': ''}, keyvals)
        keyvals = text_to_keyvalue("a:", sep=':')
        self.assertDictEqual({'a': ''}, keyvals)

    def test_wrong_lines(self):
        # we consider no key-value separator as invalid line
        # and we silently ignore these
        keyvals = text_to_keyvalue("a", sep=':',
                                   skip_invalid=True, skip_empty=False)
        self.assertDictEqual({}, keyvals)

        self.assertRaises(ValueError, text_to_keyvalue, "a", sep=':',
                          skip_invalid=False, skip_empty=False)

        # text_to_keyvalue considers the empty string as valid input
        keyvals = text_to_keyvalue("", sep=':',
                                   skip_invalid=False, skip_empty=False)
        self.assertDictEqual({}, keyvals)

        self.assertRaises(ValueError, text_to_keyvalue, "\n", sep=':',
                          skip_invalid=True, skip_empty=False)

        keyvals = text_to_keyvalue("a\n\n", sep=':',
                                   skip_invalid=True, skip_empty=True)
        self.assertDictEqual({}, keyvals)

    def test_separators(self):
        keyvals = text_to_keyvalue("a=a;b;c", sep='=', val_sep=';')
        self.assertDictEqual({'a': ['a', 'b', 'c']}, keyvals)
        keyvals = text_to_keyvalue("a 1;2;3", sep=' ', val_sep=';')
        self.assertDictEqual({'a': [1, 2, 3]}, keyvals)
        # spaces as key-value separator and values separators
        # this should work (e.g. because of : in DMS),
        # although it does not support stripping (we don't merge separators)
        keyvals = text_to_keyvalue("a 1 2 3", sep=' ', val_sep=' ')
        self.assertDictEqual({'a': [1, 2, 3]}, keyvals)

    #def test_projection_files(self):

# obtained by r.univar elevation -g
# floats removed
R_UNIVAR_KEYVAL = """n=2025000
null_cells=57995100
cells=60020100
min=55.5787925720215
max=156.329864501953
range=100.751071929932
mean=110.375440275606
mean_of_abs=110.375440275606
stddev=20.3153233205981
variance=412.712361620436
coeff_var=18.4056555243368
sum=223510266.558102
"""

# obtained by r.univar elevation -g
# floats removed
R_UNIVAR_KEYVAL_INT = """n=2025000
null_cells=57995100
cells=60020100
"""

R_UNIVAR_KEYVAL_INT_DICT = {'n': 2025000,
                            'null_cells': 57995100, 'cells': 60020100}


class TestComapreProjections(TestCase):

    def test_compare_proj_info(self):
        self.assertTrue(proj_info_equals(PROJ_INFO_TEXT_1, PROJ_INFO_TEXT_2))
        self.assertTrue(proj_units_equals(PROJ_UNITS_TEXT_1, PROJ_UNITS_TEXT_2))


class TestParseKeyvalue(TestCase):

    def test_shell_script_style(self):

        self.assertDictEqual(parse_key_val(R_UNIVAR_KEYVAL_INT, val_type=int),
                             R_UNIVAR_KEYVAL_INT_DICT)


R_UNIVAR_ELEVATION = """n=2025000
null_cells=57995100
cells=60020100
min=55.5787925720215
max=156.329864501953
range=100.751071929932
mean=110.375440275606
mean_of_abs=110.375440275606
stddev=20.3153233205981
variance=412.712361620436
coeff_var=18.4056555243368
sum=223510266.558102
first_quartile=94.79
median=108.88
third_quartile=126.792
percentile_90=138.66
"""

R_UNIVAR_ELEVATION_ROUNDED = """n=2025000
null_cells=57995100
cells=60020100
min=55.5788
max=156.33
range=100.751
mean=110.375
mean_of_abs=110.375
stddev=20.3153
variance=412.712
coeff_var=18.4057
sum=223510266.558
first_quartile=94.79
median=108.88
third_quartile=126.792
percentile_90=138.66
"""

R_UNIVAR_ELEVATION_SUBSET = """n=2025000
null_cells=57995100
cells=60020100
min=55.5787925720215
max=156.329864501953
"""


class TestRasterMapComparisons(TestCase):

    def test_compare_univars(self):
        self.assertTrue(keyvalue_equals(text_to_keyvalue(R_UNIVAR_ELEVATION,
                                                          sep='='),
                                         text_to_keyvalue(R_UNIVAR_ELEVATION,
                                                          sep='='),
                                         precision=0))
        self.assertFalse(keyvalue_equals(text_to_keyvalue(R_UNIVAR_ELEVATION,
                                                           sep='='),
                                          text_to_keyvalue(R_UNIVAR_ELEVATION_SUBSET,
                                                           sep='='),
                                          precision=0))

    def test_compare_univars_subset(self):
        self.assertTrue(keyvalue_equals(text_to_keyvalue(R_UNIVAR_ELEVATION_SUBSET,
                                                          sep='='),
                                         text_to_keyvalue(R_UNIVAR_ELEVATION,
                                                          sep='='),
                                         a_is_subset=True, precision=0))
        self.assertFalse(keyvalue_equals(text_to_keyvalue(R_UNIVAR_ELEVATION,
                                                           sep='='),
                                          text_to_keyvalue(R_UNIVAR_ELEVATION_SUBSET,
                                                           sep='='),
                                          a_is_subset=True, precision=0))

    def test_compare_univars_rounded(self):
        self.assertTrue(keyvalue_equals(text_to_keyvalue(R_UNIVAR_ELEVATION,
                                                          sep='='),
                                         text_to_keyvalue(R_UNIVAR_ELEVATION_ROUNDED,
                                                          sep='='),
                                         precision=0.001))

CORRECT_LINES = [
    "null_cells=57995100",
    "cells=60020100",
    "min=55.5787925720215",
    "max=156.329864501953"
]

INCORRECT_LINES = [
    "null_cells=579951",
    "cells=60020100",
    "min=5.5787925720215",
    "max=156.329864501953"
]


class TestMd5Sums(TestCase):
    r"""

    To create MD5 which is used for testing use:

    .. code: sh
    $ cat > test.txt << EOF
    null_cells=57995100
    cells=60020100
    min=55.5787925720215
    max=156.329864501953
    EOF
    $ md5sum test.txt
    9dd6c4bb9d2cf6051b12f4b5f9d70523  test.txt
    """

    correct_md5sum = '9dd6c4bb9d2cf6051b12f4b5f9d70523'
    correct_file_name_platform_nl = 'md5_sum_correct_file_platform_nl'
    correct_file_name_unix_nl = 'md5_sum_correct_file_unix_nl'
    wrong_file_name = 'md5_sum_wrong_file'

    @classmethod
    def setUpClass(cls):
        with open(cls.correct_file_name_platform_nl, 'w') as f:
            for line in CORRECT_LINES:
                # \n should be converted to platform newline
                f.write(line + '\n')
        with open(cls.correct_file_name_unix_nl, 'w') as f:
            for line in CORRECT_LINES:
                # binary mode will write pure \n
                f.write(line + '\n')
        with open(cls.wrong_file_name, 'w') as f:
            for line in INCORRECT_LINES:
                # \n should be converted to platform newline
                f.write(line + '\n')

    @classmethod
    def tearDownClass(cls):
        try_remove(cls.correct_file_name_platform_nl)
        try_remove(cls.correct_file_name_unix_nl)
        try_remove(cls.wrong_file_name)

    def test_text_file_binary(self):
        r"""File with ``\n`` (LF) newlines as binary (MD5 has ``\n``)."""
        self.assertEqual(file_md5(self.correct_file_name_unix_nl),
                         self.correct_md5sum,
                         msg="MD5 sums different")

    def test_text_file_platfrom(self):
        r"""Text file with platform dependent newlines"""
        self.assertEqual(text_file_md5(self.correct_file_name_platform_nl),
                         self.correct_md5sum,
                         msg="MD5 sums different")

    def test_text_file_unix(self):
        r"""Text file with ``\n`` (LF) newlines"""
        self.assertEqual(text_file_md5(self.correct_file_name_unix_nl),
                         self.correct_md5sum,
                         msg="MD5 sums different")

    def test_text_file_different(self):
        r"""Text file with ``\n`` (LF) newlines"""
        self.assertNotEqual(text_file_md5(self.wrong_file_name),
                            self.correct_md5sum,
                            msg="MD5 sums must be different")


if __name__ == '__main__':
    test()
