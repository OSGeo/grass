"""Tests of start_command function family in nc location"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import parse_command, start_command, PIPE
from grass.script.utils import parse_key_val

LOCATION = "nc"


class TestPythonKeywordsInParameters(TestCase):
    """Tests additional underscore syntax which helps to avoid Python keywords

    It works the same for keywords, buildins and any names.
    """

    raster = "elevation"

    # fresh region for each test function
    def setUp(self):
        self.use_temp_region()

    def tearDown(self):
        self.del_temp_region()

    def test_prefixed_underscore(self):
        proc = start_command("g.region", _raster=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEqual(returncode, 0, msg="Underscore as prefix was not accepted")
        self.assertNotIn(b"_raster", stderr)

    def test_suffixed_underscore(self):
        proc = start_command("g.region", raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEqual(
            returncode,
            0,
            msg="Underscore as suffix was not accepted, stderr is:\n%s" % stderr,
        )
        self.assertNotIn(b"raster_", stderr)

    def test_multiple_underscores(self):
        proc = start_command("g.region", _raster_=self.raster, stderr=PIPE)
        stderr = proc.communicate()[1]
        returncode = proc.poll()
        self.assertEqual(returncode, 1, msg="Underscore at both sides was accepted")
        self.assertIn(b"raster", stderr)


class TestParseCommand(TestCase):
    """Tests parse_command"""

    def test_parse_default(self):
        result = parse_command("r.info", map="elevation", flags="g")
        self.assertTrue(
            isinstance(result, dict) and isinstance(result.get("north"), str)
        )
        result_2 = parse_command("r.info", map="elevation", flags="g", delimiter="=")
        self.assertDictEqual(result, result_2)
        result_3 = parse_command(
            "r.info", map="elevation", flags="g", parse=(parse_key_val, {"sep": "="})
        )
        self.assertDictEqual(result, result_3)

    def test_parse_format_json(self):
        result = parse_command(
            "r.what", map="elevation", coordinates=(640000, 220000), format="json"
        )
        self.assertTrue(
            isinstance(result, list)
            and isinstance(result[0].get("easting"), (int, float))
        )

    def test_parse_format_csv(self):
        reference = parse_command("v.db.select", map="zipcodes", format="json")[
            "records"
        ]
        result = parse_command("v.db.select", map="zipcodes", format="csv")
        self.assertListEqual(list(reference[0].keys()), list(result[0].keys()))


if __name__ == "__main__":
    test()
