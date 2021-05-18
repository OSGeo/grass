############################################################################
#
# MODULE:       Test of v.db.select
# AUTHOR(S):    Vaclav Petras <wenzeslaus gmail com>
# PURPOSE:      Test parsing and structure of CSV and JSON outputs
# COPYRIGHT:    (C) 2021 by Vaclav Petras the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

"""Test parsing and structure of CSV and JSON outputs from v.db.select"""

import json
import csv
import itertools
import io

import grass.script as gs

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

POINTS = """\
17.46938776,18.67346939,143,1,Big Hill,2
20.93877551,17.44897959,125,2,Small Hill,2
18.89795918,14.18367347,130,3,1,3
18.89795918,14.18367347,130,4,1,3
15.91836735,10.67346939,126,5,1,3
15.91836735,10.67346939,126,6,1,3
15.91836735,10.67346939,126,7,1,3
15.91836735,10.67346939,126,8,1,3
15.91836735,10.67346939,126,9,1,3
"""


class JsonTest(TestCase):
    """Test case for JSON parsing and structure.

    Tests that JSON can be loaded properly by JSON reader and has expected structure.
    Standard grass.script is used for testing to mimic actual use.
    """

    # Setup variables to be used for outputs
    vector_points = "points"

    @classmethod
    def setUpClass(cls):
        cls.runModule(
            "v.in.ascii",
            input="-",
            stdin_=POINTS,
            flags="z",
            z=3,
            cat=0,
            separator="comma",
            output=cls.vector_points,
            columns="x double precision, y double precision,"
            " z double precision, owner_id integer,"
            " place_name text, num_buildings integer",
        )
        # Ensure presence of some challenging characters in a safe and transparent way.
        # This assumes that v.db.update value processing is little more straightforward
        # than v.in.ascii parsing.
        # NULL value
        cls.runModule(
            "v.db.update",
            map=cls.vector_points,
            layer=1,
            column="place_name",
            query_column="NULL",
            where="owner_id = 3",
        )
        # Double quotes pair
        cls.runModule(
            "v.db.update",
            map=cls.vector_points,
            layer=1,
            column="place_name",
            value='The "Great" Place',
            where="owner_id = 4",
        )
        # Single single quote (used as apostrophe)
        cls.runModule(
            "v.db.update",
            map=cls.vector_points,
            layer=1,
            column="place_name",
            query_column='"Joan\'s Place"',
            where="owner_id = 5",
        )
        # Pipe
        cls.runModule(
            "v.db.update",
            map=cls.vector_points,
            layer=1,
            column="place_name",
            value="Bright Hall|BLDG209",
            where="owner_id = 6",
        )
        # Comma
        cls.runModule(
            "v.db.update",
            map=cls.vector_points,
            layer=1,
            column="place_name",
            value="Raleigh, NC, USA",
            where="owner_id = 7",
        )
        # Colon and square brackets
        cls.runModule(
            "v.db.update",
            map=cls.vector_points,
            layer=1,
            column="place_name",
            value="Building: GeoLab[5]",
            where="owner_id = 8",
        )
        # Newline
        cls.runModule(
            "v.db.update",
            map=cls.vector_points,
            layer=1,
            column="place_name",
            query_column="'892 Long Street' || char(10) || 'Raleigh NC 29401'",
            where="owner_id = 9",
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region and generated data"""
        # cls.runModule("g.remove", flags="f", type="vector", name=cls.vector_points)

    def test_csv_loads(self):
        """"""
        for delimeter, null_value in itertools.product(
            [None, ",", ";", "\t", "|"], [None, "NULL"]
        ):
            text = gs.read_command(
                "v.db.select",
                map=self.vector_points,
                format="csv",
                separator=delimeter,
                null_value=null_value,
            )
            # This covers the defaults for v.db.select.
            if delimeter is None:
                delimeter = ","
            if null_value is None:
                null_value = ""
            io_string = io.StringIO(text)
            reader = csv.DictReader(
                io_string,
                delimiter=delimeter,
                quotechar='"',
                doublequote=True,
                lineterminator="\n",
                strict=True,
            )
            data = [row for row in reader]
            self.assertEqual(data[2]["place_name"], null_value)
            self.assertEqual(data[3]["place_name"], 'The "Great" Place')
            self.assertEqual(data[5]["place_name"], "Bright Hall|BLDG209")
            self.assertEqual(data[6]["place_name"], "Raleigh, NC, USA")
            self.assertEqual(data[8]["place_name"], "892 Long Street\nRaleigh NC 29401")

    def test_json_loads(self):
        """"""
        text = gs.read_command("v.db.select", map=self.vector_points, format="json")
        data = json.loads(text)
        self.assertIsNone(data[2]["place_name"])
        self.assertEqual(data[3]["place_name"], 'The "Great" Place')
        self.assertEqual(data[7]["place_name"], "Building: GeoLab[5]")
        self.assertEqual(data[8]["place_name"], "892 Long Street\nRaleigh NC 29401")


if __name__ == "__main__":
    test()
