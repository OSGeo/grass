"""
Created on Tue Jun 24 09:43:53 2014

@author: pietro
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.modules.interface.flag import Flag


class TestFlag(TestCase):
    def test_get_bash(self):
        """Test get_bash method"""
        flag = Flag(diz={"name": "a"})
        self.assertFalse(flag.value)
        self.assertEqual("", flag.get_bash())
        flag.special = True
        self.assertEqual("", flag.get_bash())
        flag.value = True
        self.assertEqual("--a", flag.get_bash())
        flag.special = False
        self.assertEqual("-a", flag.get_bash())

    def test_get_python(self):
        """Test get_python method"""
        flag = Flag(diz={"name": "a"})
        self.assertFalse(flag.value)
        self.assertEqual("", flag.get_python())
        flag.special = True
        self.assertEqual("", flag.get_python())
        flag.value = True
        self.assertEqual("a=True", flag.get_python())
        flag.special = False
        self.assertEqual("a", flag.get_python())

    def test_bool(self):
        """Test magic __bool__ method"""
        flag = Flag(diz={"name": "a"})
        flag.value = True
        self.assertTrue(bool(flag))
        flag.value = False
        self.assertFalse(bool(flag))


if __name__ == "__main__":
    test()
