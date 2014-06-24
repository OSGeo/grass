# -*- coding: utf-8 -*-
"""
Created on Tue Jun 24 09:43:53 2014

@author: pietro
"""
import unittest

from grass.script.core import get_commands
from grass.pygrass.modules.interface import Module


SKIP = []


class TestModulesMeta(type):
    def __new__(mcs, name, bases, dict):

        def gen_test(cmd):
            def test(self):
                Module(cmd)
            return test

        cmds = [c for c in sorted(list(get_commands()[0])) if c not in SKIP]
        for cmd in cmds:
            test_name = "test__%s" % cmd.replace('.', '_')
            dict[test_name] = gen_test(cmd)
        return type.__new__(mcs, name, bases, dict)


class TestModules(unittest.TestCase):
    __metaclass__ = TestModulesMeta


if __name__ == '__main__':
    unittest.main()
