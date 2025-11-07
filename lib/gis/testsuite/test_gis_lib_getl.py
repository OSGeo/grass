"""Test of gis library line reading functions

@author Vaclav Petras
"""

import ctypes
import os
import unittest
from pathlib import Path

import grass.lib.gis as libgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestNewlinesWithGetlFunctions(TestCase):
    """Test C functions G_getl() and G_getl2() from gis library"""

    @classmethod
    def setUpClass(cls):
        if os.name == "nt":
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("msvcrt"))
        else:
            cls.libc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("c"))
        cls.libc.fopen.restype = ctypes.POINTER(libgis.FILE)
        cls.libc.fopen.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        cls.file_path = Path("test.txt")

    def tearDown(self):
        self.file_path.unlink()

    def read_lines_and_assert(self, get_line_function, newline):
        """Write and read lines and then assert they are as expected"""
        lines = ["Line 1", "Line 2", "Line 3"]
        with open(self.file_path, mode="w", newline=newline) as stream:
            # Python text newline here.
            # The specific newline is added by the stream.
            stream.writelines(f"{line}\n" for line in lines)

        file_ptr = self.libc.fopen(str(self.file_path).encode("utf-8"), b"r")
        if not file_ptr:
            msg = f"Could not open file: {self.file_path}"
            raise FileNotFoundError(msg)

        try:
            buffer_size = 50
            buffer = ctypes.create_string_buffer(buffer_size)

            for line in lines:
                get_line_function(buffer, ctypes.sizeof(buffer), file_ptr)
                result = buffer.value.decode("utf-8") if buffer else None
                self.assertEqual(line, result)
        finally:
            self.libc.fclose(file_ptr)

    def test_getl_lf(self):
        r"""Check G_getl() with LF (\n)"""
        self.read_lines_and_assert(libgis.G_getl, "\n")

    @unittest.expectedFailure
    def test_getl_cr(self):
        r"""Check G_getl() with CR (\r)"""
        self.read_lines_and_assert(libgis.G_getl, "\r")

    def test_getl_crlf(self):
        r"""Check G_getl() with CRLF (\r\n)"""
        self.read_lines_and_assert(libgis.G_getl, "\r\n")

    def test_getl2_lf(self):
        r"""Check G_getl2() with LF (\n)"""
        self.read_lines_and_assert(libgis.G_getl2, "\n")

    @unittest.expectedFailure
    def test_getl2_cr(self):
        r"""Check G_getl2() with CR (\r)"""
        self.read_lines_and_assert(libgis.G_getl2, "\r")

    def test_getl2_crlf(self):
        r"""Check G_getl2() with CRLF (\r\n)"""
        self.read_lines_and_assert(libgis.G_getl2, "\r\n")


if __name__ == "__main__":
    test()
