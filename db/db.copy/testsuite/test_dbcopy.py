from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command


class TestDbCopy(TestCase):
    invect = 'zipcodes'
    orig_mapset = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'
    outable = 'my_' + invect

    @classmethod
    def setUpClass(cls):
        cls.runModule('db.connect', flags='c')

    @classmethod
    def tearDownClass(cls):
        cls.runModule('db.droptable', table=cls.outable, flags='f',
                      database='$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db')

    def test_fromtable(self):
        self.runModule('db.copy', from_database=self.orig_mapset, 
                       from_table=self.invect, to_table=self.outable,
                       overwrite=True)
        orig = read_command('db.select', table=self.invect,
                            database=self.orig_mapset)
        new = read_command('db.select', table=self.outable)
        self.assertEqual(first=orig, second=new)

    def test_select(self):
        self.runModule('db.copy', from_database=self.orig_mapset,
                       to_table=self.outable, overwrite=True,
                       select="SELECT * from {inp} WHERE NAME='RALEIGH'".format(inp=self.invect))
        orig = read_command('db.select', database=self.orig_mapset,
                            sql="SELECT * from {inp} WHERE NAME='RALEIGH'".format(inp=self.invect))
        new = read_command('db.select', table=self.outable)
        self.assertEqual(first=orig, second=new)

    def test_where(self):
        self.runModule('db.copy', from_database=self.orig_mapset,
                       to_table=self.outable, overwrite=True,
                       from_table=self.invect, where="NAME='RALEIGH'")
        orig = read_command('db.select', database=self.orig_mapset,
                            sql="SELECT * from {inp} WHERE NAME='RALEIGH'".format(inp=self.invect))
        new = read_command('db.select', table=self.outable)
        self.assertEqual(first=orig, second=new)

if __name__ == '__main__':
    test()
