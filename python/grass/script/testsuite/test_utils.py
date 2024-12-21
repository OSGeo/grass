import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows

from grass.script import utils


class EnvironChange(TestCase):
    env = {}
    NOT_FOUND = "Not found!"

    def setUp(self):
        self.original_env = {
            k: os.environ.get(k, self.NOT_FOUND) for k in self.env.keys()
        }
        for k, v in self.env.items():
            os.environ[k] = v

    def tearDown(self):
        for k in self.env.keys():
            oval = self.original_env[k]
            if oval == self.NOT_FOUND:
                os.environ.pop(k)
            else:
                os.environ[k] = oval


class LcAllC(EnvironChange):
    env = {"LC_ALL": "C"}


class TestEncode(TestCase):
    """Tests function `encode` that convert value to bytes."""

    def test_bytes(self):
        self.assertEqual(b"text", utils.encode(b"text"))

    def test_unicode(self):
        self.assertEqual(b"text", utils.encode("text"))

    @xfail_windows
    def test_bytes_garbage_in_out(self):
        """If the input is bytes we should not touch it for encoding"""
        self.assertEqual(
            b"P\xc5\x99\xc3\xad\xc5\xa1ern\xc3\xbd k\xc5\xaf\xc5\x88",
            utils.encode("Příšerný kůň"),
        )

    def test_int(self):
        """If the input is an integer return bytes"""
        self.assertRaises(TypeError, utils.encode, 1234567890)

    def test_float(self):
        """If the input is a float return bytes"""
        self.assertRaises(TypeError, utils.encode, 12345.6789)

    def test_none(self):
        """If the input is a boolean return bytes"""
        self.assertRaises(TypeError, utils.encode, None)


class TestDecode(TestCase):
    """Tests function `decode` that converts value to unicode."""

    def test_bytes(self):
        self.assertEqual("text", utils.decode(b"text"))

    def test_unicode(self):
        self.assertEqual("text", utils.decode("text"))

    def test_int(self):
        """If the input is an integer return bytes"""
        self.assertRaises(TypeError, utils.decode, 1234567890)

    def test_float(self):
        """If the input is a float return bytes"""
        self.assertRaises(TypeError, utils.decode, 12345.6789)

    def test_none(self):
        """If the input is a boolean return bytes"""
        self.assertRaises(TypeError, utils.decode, None)


class TestEncodeLcAllC(TestEncode, LcAllC):
    pass


class TestDecodeLcAllC(TestDecode, LcAllC):
    pass


if __name__ == "__main__":
    test()
