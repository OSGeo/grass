# -*- coding: utf-8 -*-
import os
import sys

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

    def test_bytes_garbage_in_out(self):
        """If the input is bytes we should not touch it for encoding"""
        self.assertEqual(b'P\xc5\x99\xc3\xad\xc5\xa1ern\xc3\xbd k\xc5\xaf\xc5\x88',
                         utils.encode('Příšerný kůň'))

    def test_int(self):
        """If the input is an integer return bytes"""
        if sys.version_info.major >= 3:
            self.assertRaises(TypeError, utils.encode, 1234567890)
        else:
            self.assertEqual('1234567890', utils.encode(1234567890))

    def test_float(self):
        """If the input is a float return bytes"""
        if sys.version_info.major >= 3:
            self.assertRaises(TypeError, utils.encode, 12345.6789)
        else:
            self.assertEqual('12345.6789', utils.encode(12345.6789))

    def test_none(self):
        """If the input is a boolean return bytes"""
        if sys.version_info.major >= 3:
            self.assertRaises(TypeError, utils.encode, None)
        else:
            self.assertEqual('None', utils.encode(None))


class TestDecode(TestCase):
    """Tests function `decode` that converts value to unicode."""

    def test_bytes(self):
        self.assertEqual(u'text', utils.decode(b'text'))

    def test_unicode(self):
        self.assertEqual(u'text', utils.decode(u'text'))

    def test_int(self):
        """If the input is an integer return bytes"""
        if sys.version_info.major >= 3:
            self.assertRaises(TypeError, utils.decode, 1234567890)
        else:
            self.assertEqual(u'1234567890', utils.decode(1234567890))

    def test_float(self):
        """If the input is a float return bytes"""
        if sys.version_info.major >= 3:
            self.assertRaises(TypeError, utils.decode, 12345.6789)
        else:
            self.assertEqual(u'12345.6789', utils.decode(12345.6789))

    def test_none(self):
        """If the input is a boolean return bytes"""
        if sys.version_info.major >= 3:
            self.assertRaises(TypeError, utils.decode, None)
        else:
            self.assertEqual(u'None', utils.decode(None))


class TestEncodeLcAllC(TestEncode, LcAllC):
    pass


class TestDecodeLcAllC(TestDecode, LcAllC):
    pass


if __name__ == '__main__':
    test()
