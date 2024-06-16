"""Test of gis library line reading functions

@author Vaclav Petras
"""

import ctypes
import pathlib
import platform

import grass.lib.gis as libgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


def load_libc_by_name(names):
    """Return loaded libc library trying the given names"""
    for name in names:
        try:
            return ctypes.CDLL(name)
        except OSError:
            pass
    for name in names:
        path = ctypes.util.find_library(name)
        if path:
            try:
                return ctypes.CDLL(path)
            except OSError:
                pass
    raise OSError("Could not load libc library")


def load_libc():
    """Return loaded libc library based on platfrom"""
    if platform.system() == "Windows":
        return ctypes.CDLL("msvcrt.dll")
    if platform.system() == "Darwin":
        return ctypes.CDLL("libc.dylib")
    names = ["libc.so.6", "libc.so", "libc"]
    return load_libc_by_name(names)


class TestNewlinesWithGetlFunctions(TestCase):
    """Test C functions G_getl() and G_getl2() from gis library"""

    @classmethod
    def setUpClass(cls):
        cls.libc = load_libc()
        cls.file_path = pathlib.Path("test.txt")

    def tearDown(self):
        self.file_path.unlink()

    def read_lines_and_assert(self, get_line_function, newline):
        """Write and read lines and then assert they are as expected"""
        lines = ["Line 1", "Line 2", "Line 3"]
        with open(self.file_path, mode="w", newline=newline) as stream:
            for line in lines:
                # Python text newline here.
                # The specific newline is added by the stream.
                stream.write(f"{line}\n")

        file_ptr = self.libc.fopen(str(self.file_path).encode("utf-8"), b"r")
        if not file_ptr:
            raise FileNotFoundError(f"Could not open file: {self.file_path}")

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

    def test_getl_cr(self):
        r"""Check G_getl() with CR (\r)"""
        self.read_lines_and_assert(libgis.G_getl, "\r")

    def test_getl_crlf(self):
        r"""Check G_getl() with CRLF (\r\n)"""
        self.read_lines_and_assert(libgis.G_getl, "\r\n")

    def test_getl2_lf(self):
        r"""Check G_getl2() with LF (\n)"""
        self.read_lines_and_assert(libgis.G_getl2, "\n")

    def test_getl2_cr(self):
        r"""Check G_getl2() with CR (\r)"""
        self.read_lines_and_assert(libgis.G_getl2, "\r")

    def test_getl2_crlf(self):
        r"""Check G_getl2() with CRLF (\r\n)"""
        self.read_lines_and_assert(libgis.G_getl2, "\r\n")


if __name__ == "__main__":
    test()
