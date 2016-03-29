# -*- coding: utf-8 -*-
import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script import utils


def change_environ(**envs):
    NOT_FOUND = 'Not found!'
    original_envs = {k: os.environ.get(k, NOT_FOUND) for k in envs.keys()}

    def work_with_env(func):
        def wrap_func(*args, **kwargs):
            # modify the environment variables
            for k, v in envs.items():
                os.environ[k] = v

            # execute
            func(*args, **kwargs)

            # restore the environment variables
            for k, v in envs.items():
                oval = original_envs[k]
                if oval == NOT_FOUND:
                    os.environ.pop(k)
                else:
                    os.environ[k] = oval
        return wrap_func
    return work_with_env


class TestEncode(TestCase):
    """Tests function `encode` that convert value to bytes."""

    def test_bytes(self):
        self.assertEqual(b'text', utils.encode(b'text'))

    def test_unicode(self):
        self.assertEqual(b'text', utils.encode(u'text'))

    @change_environ(LC_ALL='C')
    def test_bytes_LC_ALL_C(self):
        self.assertEqual(b'text', utils.encode(b'text'))

    @change_environ(LC_ALL='C')
    def test_unicode_LC_ALL_C(self):
        self.assertEqual(b'text', utils.encode(u'text'))


class TestDecode(TestCase):
    """Tests function `encode` that convert value to unicode."""

    def test_bytes(self):
        self.assertEqual(u'text', utils.decode(b'text'))

    def test_unicode(self):
        self.assertEqual(u'text', utils.decode(u'text'))

    @change_environ(LC_ALL='C')
    def test_bytes_LC_ALL_C(self):
        self.assertEqual(u'text', utils.decode(b'text'))

    @change_environ(LC_ALL='C')
    def test_unicode_LC_ALL_C(self):
        self.assertEqual(u'text', utils.decode(u'text'))




if __name__ == '__main__':
    test()