# -*- coding: utf-8 -*-
"""
Created on Tue Jun 24 09:43:53 2014

@author: pietro
"""
from __future__ import unicode_literals
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.modules.interface.flag import Flag


class TestFlag(TestCase):
    def test_get_bash(self):
        """Test get_bash method"""
        flag = Flag(diz=dict(name='a'))
        self.assertFalse(flag.value)
        self.assertEqual('', flag.get_bash())
        flag.special = True
        self.assertEqual('', flag.get_bash())
        flag.value = True
        self.assertEqual('--a', flag.get_bash())
        flag.special = False
        self.assertEqual('-a', flag.get_bash())

    def test_get_python(self):
        """Test get_python method"""
        flag = Flag(diz=dict(name='a'))
        self.assertFalse(flag.value)
        self.assertEqual('', flag.get_python())
        flag.special = True
        self.assertEqual('', flag.get_python())
        flag.value = True
        self.assertEqual('a=True', flag.get_python())
        flag.special = False
        self.assertEqual('a', flag.get_python())

    def test_bool(self):
        """Test magic __bool__ method"""
        flag = Flag(diz=dict(name='a'))
        flag.value = True
        self.assertTrue(True if flag else False)
        flag.value = False
        self.assertFalse(True if flag else False)


if __name__ == '__main__':
    test()
