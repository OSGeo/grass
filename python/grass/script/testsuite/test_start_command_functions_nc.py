"""Tests of start_command function family in nc location"""

LOCATION = 'nc'

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import start_command, PIPE


class TestPythonKeywordsInParameters(TestCase):
    """Tests additional underscore syntax which helps to avoid Python keywords

    It works the same for keywords, buildins and any names.
    """

    raster = 'elevation'

    # fresh region for each test function
    def setUp(self):
        self.use_temp_region()

    def tearDown(self):
        self.del_temp_region()

    def test_prefixed_underscore(self):
        proc = start_command(
            'g.region', _raster=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEquals(returncode, 0,
            msg="Underscore as prefix was not accepted")
        self.assertNotIn(b'_raster', stderr)

    def test_suffixed_underscore(self):
        proc = start_command(
            'g.region', raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEquals(returncode, 0,
            msg="Underscore as suffix was not accepted, stderr is:\n%s" % stderr)
        self.assertNotIn(b'raster_', stderr)

    def test_multiple_underscores(self):
        proc = start_command(
            'g.region', _raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEquals(returncode, 1,
            msg="Underscore at both sides was accepted")
        self.assertIn(b'raster', stderr)

if __name__ == '__main__':
    test()
