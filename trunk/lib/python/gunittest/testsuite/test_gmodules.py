# -*- coding: utf-8 -*-

import subprocess

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import (call_module, CalledModuleError)

G_REGION_OUTPUT = """...
n=...
s=...
w=...
e=...
nsres=...
ewres=...
rows=...
cols=...
cells=...
"""


class TestCallModuleFunction(TestCase):

    def test_output(self):
        output = call_module('g.region', flags='pg')
        self.assertLooksLike(output, G_REGION_OUTPUT)

    def test_input_output(self):
        output = call_module('m.proj', flags='i', input='-', stdin="50.0 41.5")
        self.assertLooksLike(output, '...|...\n')

    def test_no_output(self):
        output = call_module('m.proj', flags='i', input='-', stdin="50.0 41.5",
                             capture_stdout=False)
        self.assertIsNone(output)

    def test_merge_stderr(self):
        output = call_module('m.proj', flags='i', input='-', stdin="50.0 41.5",
                             verbose=True,
                             merge_stderr=True)
        self.assertLooksLike(output, '...+proj=longlat +datum=WGS84...')
        self.assertLooksLike(output, '...|...\n')

    def test_merge_stderr_with_wrong_stdin_stderr(self):
        self.assertRaises(ValueError,
                          call_module,
                          'm.proj', flags='i', input='-', stdin="50.0 41.5",
                          verbose=True,
                          merge_stderr=True, capture_stdout=False)
        self.assertRaises(ValueError,
                          call_module,
                          'm.proj', flags='i', input='-', stdin="50.0 41.5",
                          verbose=True,
                          merge_stderr=True, capture_stderr=False)
        self.assertRaises(ValueError,
                          call_module,
                          'm.proj', flags='i', input='-', stdin="50.0 41.5",
                          verbose=True,
                          merge_stderr=True,
                          capture_stdout=False, capture_stderr=False)

    def test_wrong_module_params(self):
        self.assertRaises(CalledModuleError,
                          call_module,
                          'g.region', aabbbccc='notexist')

    def test_module_input_param_wrong(self):
        self.assertRaises(ValueError,
                          call_module,
                          'm.proj', flags='i', input='does_not_exist',
                          stdin="50.0 41.5")

    def test_missing_stdin_with_input_param(self):
        self.assertRaises(ValueError,
                          call_module,
                          'm.proj', flags='i', input='-')

    def test_wrong_usage_of_popen_like_interface(self):
        self.assertRaises(ValueError,
                          call_module,
                          'm.proj', flags='i', input='-',
                          stdin=subprocess.PIPE)
        self.assertRaises(TypeError,
                          call_module,
                          'm.proj', flags='i', input='-', stdin="50.0 41.5",
                          stdout='any_value_or_type_here')
        self.assertRaises(TypeError,
                          call_module,
                          'm.proj', flags='i', input='-', stdin="50.0 41.5",
                          stderr='any_value_or_type_here')


if __name__ == '__main__':
    test()
