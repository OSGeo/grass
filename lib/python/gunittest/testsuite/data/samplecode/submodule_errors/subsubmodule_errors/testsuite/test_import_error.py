# -*- coding: utf-8 -*-

# comment to get rid of the wrong import
# (if it is imported before all tests start and everything would fail)
#import this_module_or_package_does_not_exists__testing_import_error

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestNeverCalled(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        self.assertFalse("This should not be called"
                         " if we are testing failed import."
                         " It is all right if this fails and the wrong"
                         " import is commented.")


if __name__ == '__main__':
    test()
