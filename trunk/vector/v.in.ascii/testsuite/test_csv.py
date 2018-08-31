"""Test v.in.ascii CSV capabilities

:author: Vaclav Petras
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command


INPUT_NOQUOTES = """Id,POINT_X,POINT_Y,Category,ED field estimate
100,437343.6704,4061363.41525,High Erosion,Low Deposition
101,453643.127906,4050070.29852,High Erosion,Low Erosion
102,454903.605427,4049480.80568,High Erosion,High Erosion
105,437734.838807,4060493.98315,High Erosion,Low Erosion
107,450833.019732,4048207.02664,High Erosion,Low Erosion
"""

INPUT_DOUBLEQUOTES = """Id,POINT_X,POINT_Y,Category,"ED field estimate"
100,437343.6704,4061363.41525,"High Erosion","Low Deposition"
101,453643.127906,4050070.29852,"High Erosion","Low Erosion"
102,454903.605427,4049480.80568,"High Erosion","High Erosion"
105,437734.838807,4060493.98315,"High Erosion","Low Erosion"
107,450833.019732,4048207.02664,"High Erosion","Low Erosion"
"""

INPUT_TSV = """Id\tPOINT_X\tPOINT_Y\tCategory\t"ED field estimate"
100\t437343.6704\t4061363.41525\t"High Erosion"\t"Low Deposition"
101\t453643.127906\t4050070.29852\t"High Erosion"\t"Low Erosion"
102\t454903.605427\t4049480.80568\t"High Erosion"\t"High Erosion"
105\t437734.838807\t4060493.98315\t"High Erosion"\t"Low Erosion"
107\t450833.019732\t4048207.02664\t"High Erosion"\t"Low Erosion"
"""

INPUT_UNCOMMON = """Id@POINT_X@POINT_Y@Category@^ED field estimate^
100@437343.6704@4061363.41525@^High Erosion^@^Low Deposition^
101@453643.127906@4050070.29852@^High Erosion^@^Low Erosion^
102@454903.605427@4049480.80568@^High Erosion^@^High Erosion^
105@437734.838807@4060493.98315@^High Erosion^@^Low Erosion^
107@450833.019732@4048207.02664@^High Erosion^@^Low Erosion^
"""

TABLE_1 = """cat|x|y|ed_cat|field_estimate
100|437343.6704|4061363.41525|High Erosion|Low Deposition
101|453643.127906|4050070.29852|High Erosion|Low Erosion
102|454903.605427|4049480.80568|High Erosion|High Erosion
105|437734.838807|4060493.98315|High Erosion|Low Erosion
107|450833.019732|4048207.02664|High Erosion|Low Erosion
"""


class SimpleCsvTestCase(TestCase):

    xyvector = 'yxvetor_test'

    def tearDown(self):
        """Remove the vector map after each test method"""
        self.runModule('g.remove', flags='f', type='vector',
                       name=self.xyvector)

    def test_no_text_delimeter(self):
        """Test type of resulting map"""
        self.assertModule(
            'v.in.ascii', input='-', output=self.xyvector,
            separator='comma', skip=1, x=2, y=3, cat=1,
            columns="cat int, x double, y double,"
                    " ed_cat varchar(20), field_estimate varchar(20)",
            stdin_=INPUT_NOQUOTES)

        category = read_command('v.db.select', map=self.xyvector,
                                separator='pipe')
        self.assertEqual(first=TABLE_1.replace('\n', os.linesep),
                         second=category,
                         msg="Attribute table has wrong entries")

    def test_text_delimeter(self):
        """Test loading CSV with text delimiter

        Text delimiter added in r63581
        """
        self.assertModule(
            'v.in.ascii', input='-', output=self.xyvector,
            separator='comma', text='doublequote',
            skip=1, x=2, y=3, cat=1,
            columns="cat int, x double, y double,"
                    " ed_cat varchar(20), field_estimate varchar(20)",
            stdin_=INPUT_DOUBLEQUOTES)

        category = read_command('v.db.select', map=self.xyvector,
                                separator='pipe')
        self.assertEqual(first=TABLE_1.replace('\n', os.linesep),
                         second=category,
                         msg="Attribute table has wrong entries")
        # TODO: a general method to compare attribute tables? (might need to solve because of floats)
        # TODO: standardize string strip? perhaps discourage, it messes up the diff
        # TODO: use replace solution for newlines in lib (compare to current one)

    def test_tsv(self):
        """Test loading TSV (CSV with tab as delim)

        Using double quote character for quote.
        """
        self.assertModule(
            'v.in.ascii', input='-', output=self.xyvector,
            separator='tab', text='"',
            skip=1, x=2, y=3, cat=1,
            columns="cat int, x double, y double,"
                    " ed_cat varchar(20), field_estimate varchar(20)",
            stdin_=INPUT_TSV)

        category = read_command('v.db.select', map=self.xyvector,
                                separator='pipe')
        self.assertEqual(first=TABLE_1.replace('\n', os.linesep),
                         second=category,
                         msg="Attribute table has wrong entries")

    def test_uncommon_delims(self):
        """Test loading CSV with uncommon delimiters"""
        self.assertModule(
            'v.in.ascii', input='-', output=self.xyvector,
            separator='@', text='^',
            skip=1, x=2, y=3, cat=1,
            columns="cat int, x double, y double,"
                    " ed_cat varchar(20), field_estimate varchar(20)",
            stdin_=INPUT_UNCOMMON)

        category = read_command('v.db.select', map=self.xyvector,
                                separator='pipe')
        self.assertEqual(first=TABLE_1.replace('\n', os.linesep),
                         second=category,
                         msg="Attribute table has wrong entries")


INPUT_DELIM_IN_TEXT = """Id,POINT_X,POINT_Y,Category,"ED field estimate"
100,437343.6704,4061363.41525,"High Erosion, Low Canopy","Low Deposition, Low Canopy"
101,453643.127906,4050070.29852,"High Erosion, High Canopy","Low Erosion, Low Canopy"
102,454903.605427,4049480.80568,"High Erosion, High Canopy","High Erosion, Low Canopy"
105,437734.838807,4060493.98315,"High Erosion, Low Canopy","Low Erosion, High Canopy"
107,450833.019732,4048207.02664,"High Erosion, Low Canopy","Low Erosion, High Canopy"
"""

TABLE_2 = """cat|x|y|ed_cat|field_estimate
100|437343.6704|4061363.41525|High Erosion, Low Canopy|Low Deposition, Low Canopy
101|453643.127906|4050070.29852|High Erosion, High Canopy|Low Erosion, Low Canopy
102|454903.605427|4049480.80568|High Erosion, High Canopy|High Erosion, Low Canopy
105|437734.838807|4060493.98315|High Erosion, Low Canopy|Low Erosion, High Canopy
107|450833.019732|4048207.02664|High Erosion, Low Canopy|Low Erosion, High Canopy
"""


class AdvancedCsvTestCase(TestCase):

    xyvector = 'yxvetor_test'

    def tearDown(self):
        """Remove the vector map after each test method"""
        self.runModule('g.remove', flags='f', type='vector',
                       name=self.xyvector)

    def test_delimeter_in_text(self):
        """Test loading CSV with delimiter in text

        Text delimiter added in r63581
        """
        self.assertModule(
            'v.in.ascii', input='-', output=self.xyvector,
            separator='comma', text='doublequote',
            skip=1, x=2, y=3, cat=1,
            columns="cat int, x double, y double,"
                    " ed_cat varchar(40), field_estimate varchar(40)",
            stdin_=INPUT_DELIM_IN_TEXT)

        category = read_command('v.db.select', map=self.xyvector,
                                separator='pipe')
        self.assertEqual(first=TABLE_2.replace('\n', os.linesep),
                         second=category,
                         msg="Attribute table has wrong entries")


if __name__ == '__main__':
    test()
