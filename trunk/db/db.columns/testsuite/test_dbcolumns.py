from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command

output = """cat
OBJECTID
WAKE_ZIPCO
PERIMETER
ZIPCODE_
ZIPCODE_ID
ZIPNAME
ZIPNUM
ZIPCODE
NAME
SHAPE_Leng
SHAPE_Area
"""

class TestDbColumns(TestCase):
    invect = 'zipcodes'
    mapset = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'

    @classmethod
    def setUpClass(cls):
        cls.runModule('db.connect', flags='c')

    def test_dbcols(self):
        cols = read_command('db.columns', table=self.invect,
                            database=self.mapset)
        self.assertEqual(first=cols, second=output)

if __name__ == '__main__':
    test()
