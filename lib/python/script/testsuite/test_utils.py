# -*- coding: utf-8 -*-
import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script import utils


class EnvironChange(TestCase):
    env = dict()
    NOT_FOUND = 'Not found!'

    def setUp(self):
        self.original_env = {k: os.environ.get(k, self.NOT_FOUND)
                             for k in self.env.keys()}
        for k, v in self.env.items():
                os.environ[k] = v

    def tearDown(self):
        for k, v in self.env.items():
                oval = self.original_env[k]
                if oval == self.NOT_FOUND:
                    os.environ.pop(k)
                else:
                    os.environ[k] = oval


class LcAllC(EnvironChange):
    env = dict(LC_ALL='C')


class TestEncode(TestCase):
    """Tests function `encode` that convert value to bytes."""

    def test_bytes(self):
        self.assertEqual(b'text', utils.encode(b'text'))

    def test_unicode(self):
        self.assertEqual(b'text', utils.encode(u'text'))

    def test_bytes_grabage_in_out(self):
        """If the input is bytes we should not touch it for encoding"""
        self.assertEqual(b'Příšerný kůň', utils.encode(b'Příšerný kůň'))


class TestDecode(TestCase):
    """Tests function `encode` that convert value to unicode."""

    def test_bytes(self):
        self.assertEqual(u'text', utils.decode(b'text'))

    def test_unicode(self):
        self.assertEqual(u'text', utils.decode(u'text'))


class TestEncodeLcAllC(TestEncode, LcAllC):
    pass


class TestDecodeLcAllC(TestDecode, LcAllC):
    pass


if __name__ == '__main__':
    test()
