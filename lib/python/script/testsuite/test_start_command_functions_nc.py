"""Tests of start_command function family in nc location"""

LOCATION = 'nc'

import grass.gunittest

from grass.script.core import start_command, PIPE


class TestPythonKeywordsInParameters(grass.gunittest.TestCase):
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
            'g.region', _rast=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEquals(returncode, 0,
            msg="Undersocre as prefix was not accepted")
        self.assertNotIn('_rast', stderr)

    def test_suffixed_underscore(self):
        proc = start_command(
            'g.region', rast_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEquals(returncode, 0,
            msg="Undersocre as suffix was not accepted, stderr is:\n%s" % stderr)
        self.assertNotIn('rast_', stderr)

    def test_multiple_underscores(self):
        proc = start_command(
            'g.region', _rast_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEquals(returncode, 1,
            msg="Undersocre at both sides was accepted")
        self.assertIn('rast', stderr)

if __name__ == '__main__':
    grass.gunittest.test()
