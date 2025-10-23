"""Test of gjson library

@author Nishant Bansal
"""

from grass.gunittest.case import TestCase


class GjsonLibraryTest(TestCase):
    def test_wrapper(self):
        self.assertModule("test.gjson.lib", flags="u")


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
