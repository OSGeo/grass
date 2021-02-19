# -*- coding: utf-8 -*-

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestError(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        raise RuntimeError('Error in test function')
        self.assertTrue(True)


class TestErrorSetUp(TestCase):
    # pylint: disable=R0904

    def setUp(self):
        raise RuntimeError('Error in setUp')

    def test_something(self):
        self.assertTrue(True)


class TestErrorTearDown(TestCase):
    # pylint: disable=R0904

    def tearDown(self):
        raise RuntimeError('Error in tearDown')

    def test_something(self):
        self.assertTrue(True)


class TestErrorClassSetUp(TestCase):
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        raise RuntimeError('Error in setUpClass')

    def test_something(self):
        self.assertTrue(True)


class TestErrorClassTearDown(TestCase):
    # pylint: disable=R0904

    @classmethod
    def tearDownClass(cls):
        raise RuntimeError('Error in tearDownClass')

    def test_something(self):
        self.assertTrue(True)


if __name__ == '__main__':
    test()
