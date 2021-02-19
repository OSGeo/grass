# -*- coding: utf-8 -*-
from __future__ import print_function
from unittest import TestCase, main


class TestUnittestSuccessVerboseSetUp(TestCase):
    # pylint: disable=R0904

    def setUp(self):
        print("print from setUp")

    def tearDown(self):
        print("print from tearDown")

    def test_something(self):
        self.assertTrue(True, msg="This should not fail")

    def test_something_failing(self):
        self.assertTrue(False, msg="This should fail")


class TestUnittestSuccessVerboseClassSetUp(TestCase):
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        print("print from setUpClass")

    @classmethod
    def tearDownClass(cls):
        print("print from tearDownClass")

    def test_something(self):
        self.assertTrue(True, msg="This should not fail")

    def test_something_failing(self):
        self.assertTrue(False, msg="This should fail")

if __name__ == '__main__':
    main()
