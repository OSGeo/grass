############################################################################
#
# MODULE:       Test of v.db.select
# AUTHOR(S):    Tomas Zigo <tomas zigo slovanet sk>
# PURPOSE:      Test query with defined columns (column name is reserved DB
#               keyword)
# COPYRIGHT:    (C) 2021-2024 by Tomas Zigo the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

"""Test query with defined columns (column name is reserved DB keyword)"""

import grass.script as gs

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

POINTS = """\
17.46938776,18.67346939,1
20.93877551,17.44897959,2
"""


class QueryWithDefinedColumnsTest(TestCase):
    """Test case for query with defined columns (column name is reserved
    DB keyword)
    """

    # Setup variables to be used for outputs
    vector_points = "points"

    @classmethod
    def setUpClass(cls):
        """Create points with reserved DB keyword column name"""
        cls.runModule(
            "v.in.ascii",
            input="-",
            stdin_=POINTS,
            cat=0,
            separator="comma",
            output=cls.vector_points,
            columns='x double precision, y double precision, "order" integer',
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the test data"""
        cls.runModule("g.remove", flags="f", type="vector", name=cls.vector_points)

    def test_query_with_defined_columns(self):
        """Test query with defined columns param arg"""
        data = gs.read_command(
            "v.db.select",
            map=self.vector_points,
            columns="cat,order",
        )
        self.assertEqual(data, "cat|order\n1|1\n2|2\n")


if __name__ == "__main__":
    test()
