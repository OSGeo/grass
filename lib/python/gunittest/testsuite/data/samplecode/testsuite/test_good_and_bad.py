# -*- coding: utf-8 -*-

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestSuccessAndFailure(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        self.assertTrue(True, msg="This should not fail in test_good_and_bad")

    def test_something_else(self):
        self.assertTrue(True, msg="This should not fail in test_good_and_bad")

    def test_something_failing(self):
        self.assertTrue(False, msg="This failed in test_good_and_bad")

    def test_something_erroring(self):
        raise RuntimeError('Some error which was raised')
        self.assertTrue(True, msg="This should not fail in test_good_and_bad")

if __name__ == '__main__':
    test()
