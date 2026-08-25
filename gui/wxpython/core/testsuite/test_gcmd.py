from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_cmake, xfail_windows


class Rcv:
    data = b"\xc4\x81" * 10
    pos = 0

    def recv(self):
        start = self.pos
        self.pos += 5
        end = self.pos
        if end > len(self.data):
            return None
        return self.data[start:end]


class Recv_SomeTest(TestCase):
    @xfail_cmake
    @xfail_windows
    def test_decode(self):
        """
        Multibyte chars should not be split

        A test case for bug #2720
        https://github.com/OSGeo/grass/issues/2720
        """
        p = Rcv()
        recv_some(p, e=0)


if __name__ == "__main__":
    try:
        from grass.script.setup import set_gui_path

        set_gui_path()

        from core.gcmd import recv_some

        test()
    except ModuleNotFoundError:
        # Tests can not run if wx is absent
        pass
