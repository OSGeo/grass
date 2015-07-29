"""Tests of start_command function family (location independent)"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import start_command, PIPE


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

if __name__ == '__main__':
    test()
