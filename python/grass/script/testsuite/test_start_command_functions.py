"""
Tests of start_command function family (location independent)
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import xfail_windows

from grass.script import start_command, PIPE, run_command, write_command, read_command
from grass.script.utils import encode


class TestPythonKeywordsInParameters(TestCase):
    """Tests additional underscore syntax which helps to avoid Python keywords

    It works the same for keywords, buildins and any names.
    """

    raster = "does_not_exist"

    def test_prefixed_underscore(self):
        proc = start_command("g.region", _raster=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        self.assertNotIn("_raster", stderr)
        self.assertIn(
            self.raster, stderr, msg="Raster map name should appear in the error output"
        )

    def test_suffixed_underscore(self):
        proc = start_command("g.region", raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        self.assertNotIn("raster_", stderr)
        self.assertIn(
            self.raster, stderr, msg="Raster map name should appear in the error output"
        )

    def test_multiple_underscores(self):
        proc = start_command("g.region", _raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEqual(returncode, 1)
        self.assertIn("raster", stderr)


class TestPythonModuleWithUnicodeParameters(TestCase):
    """Tests if unicode works in parameters of Python modules

    This in fact tests also the `parser()` function (original motivation
    for this tests).

    Using g.search.module because it takes any option values.
    """

    def test_python_module_ascii(self):
        """This tests if Python module works"""
        run_command("g.search.modules", keyword="Priserny kun", encoding=None)

    def test_python_module_ascii_with_encoding(self):
        """This tests if Python module works with user-specified encoding"""
        run_command("g.search.modules", keyword="Priserny kun", encoding="utf-16")

    def test_python_module_czech_nonascii(self):
        """This likely fails on non-UTF-8 systems (i.e. MS Win)"""
        run_command("g.search.modules", keyword="Příšerný kůň")

    def test_python_module_czech_unicode(self):
        """This likely fails on non-UTF-8 systems (i.e. MS Win)"""
        run_command("g.search.modules", keyword="Příšerný kůň")


class TestPythonModuleWithStdinStdout(TestCase):
    raster = "rand_raster"

    @classmethod
    def setUpClass(cls):
        cls.runModule("g.region", s=0, n=10, w=0, e=10, res=1)
        cls.runModule(
            "r.mapcalc",
            expression="{} = rand(1, 5)".format(cls.raster),
            seed=1,
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        cls.runModule("g.remove", type="raster", name=cls.raster, flags="f")

    def test_write_read_labels_str(self):
        """This tests if standard string works"""
        write_command(
            "r.category",
            map=self.raster,
            rules="-",
            stdin="1:kůň\n2:kráva\n3:ovečka\n4:býk",
            separator=":",
        )
        res = read_command("r.category", map=self.raster, separator=":").strip()
        self.assertEqual(res, "1:kůň\n2:kráva\n3:ovečka\n4:býk")
        self.assertIsInstance(res, str)

    @xfail_windows
    def test_write_bytes_read_str(self):
        """This test backwards compatibility when writing bytes"""
        write_command(
            "r.category",
            map=self.raster,
            rules="-",
            stdin=encode("1:kůň\n2:kráva\n3:ovečka\n4:býk"),
            separator=":",
        )
        res = read_command("r.category", map=self.raster, separator=":").strip()
        self.assertEqual(res, "1:kůň\n2:kráva\n3:ovečka\n4:býk")
        self.assertIsInstance(res, str)


if __name__ == "__main__":
    test()
