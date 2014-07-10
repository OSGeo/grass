# -*- coding: utf-8 -*-

# TODO: change to GrassTestCase
from unittest import TestCase


class TestSuccessVerboseSetUp(TestCase):
    # pylint: disable=R0904

    def setUp(self):
        print "print from setUp"

    def tearDown(self):
        print "print from tearDown"

    def test_something(self):
        self.assertTrue(True)


class TestSuccessVerboseClassSetUp(TestCase):
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        print "print from setUpClass"

    @classmethod
    def tearDownClass(cls):
        print "print from tearDownClass"

    def test_something(self):
        self.assertTrue(True)

if __name__ == '__main__':
    import unittest
    unittest.main()
