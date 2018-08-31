# -*- coding: utf-8 -*-
# the utf-8 is important because we do use the characters
"""Tests of start_command function family (location independent)"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import start_command, PIPE, run_command


class TestPythonKeywordsInParameters(TestCase):
    """Tests additional underscore syntax which helps to avoid Python keywords

    It works the same for keywords, buildins and any names.
    """

    raster = b'does_not_exist'

    def test_prefixed_underscore(self):
        proc = start_command(
            'g.region', _raster=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        self.assertNotIn(b'_raster', stderr)
        self.assertIn(self.raster, stderr,
            msg="Raster map name should appear in the error output")

    def test_suffixed_underscore(self):
        proc = start_command(
            'g.region', raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        self.assertNotIn(b'raster_', stderr)
        self.assertIn(self.raster, stderr,
            msg="Raster map name should appear in the error output")

    def test_multiple_underscores(self):
        proc = start_command(
            'g.region', _raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEquals(returncode, 1)
        self.assertIn(b'raster', stderr)


class TestPythonModuleWithUnicodeParameters(TestCase):
    """Tests if unicode works in parameters of Python modules

    This in fact tests also the `parser()` function (original motivation
    for this tests).

    Using g.search.module because it takes any option values.
    """

    def test_python_module_ascii(self):
        """This tests if Python module works"""
        run_command('g.search.modules', keyword=b'Priserny kun')

    def test_python_module_czech_nonascii(self):
        """This likely fails on non-UTF-8 systems (i.e. MS Win)"""
        run_command('g.search.modules', keyword=b'Příšerný kůň')

    def test_python_module_czech_unicode(self):
        """This likely fails on non-UTF-8 systems (i.e. MS Win)"""
        run_command('g.search.modules', keyword=u'Příšerný kůň')


if __name__ == '__main__':
    test()
