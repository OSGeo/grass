# -*- coding: utf-8 -*-

# comment to get rid of the wrong import
# (if it is imported before all tests start)
#import this_module_or_package_does_not_exists__testing_import_error

# TODO: change to GrassTestCase
from unittest import TestCase


class TestNeverCalled(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        self.assertFalse("This should not be called"
                         " if we are testing failed import."
                         " But it is good OK if one wrong import"
                         " would prevent other tests from running"
                         " due to the implementation of test invocation.")


if __name__ == '__main__':
    import unittest
    unittest.main()
