# -*- coding: utf-8 -*-
from grass.gunittest import TestCase, test
from grass.gunittest.gmodules import SimpleModule

out1 = """East: 634243
North: 226193
------------------------------------------------------------------
Map: test_vector 
Mapset: vector_what
Type: Area
Sq Meters: 633834.281
Hectares: 63.383
Acres: 156.624
Sq Miles: 0.2447
Layer: 1
Category: 2
Layer: 1
Category: 1
Layer: 2
Category: 3
Layer: 2
Category: 4
"""


out2 = """East: 634243
North: 226193
------------------------------------------------------------------
Map: test_vector 
Mapset: vector_what
Type: Area
Sq Meters: 633834.281
Hectares: 63.383
Acres: 156.624
Sq Miles: 0.2447
Layer: 1
Category: 2

Driver: sqlite
Database: /home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table: t1
Key column: cat_
Layer: 1
Category: 1

Driver: sqlite
Database: /home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table: t1
Key column: cat_
cat_ : 1
text : xxx
number : 6
Layer: 2
Category: 3

Driver: sqlite
Database: /home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table: t2
Key column: cat_
Layer: 2
Category: 4

Driver: sqlite
Database: /home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table: t2
Key column: cat_
cat_ : 4
text : yyy
number : 8.09
"""

out3 = """East=634243
North=226193

Map=test_vector
Mapset=vector_what
Type=Area
Sq_Meters=633834.281
Hectares=63.383
Acres=156.624
Sq_Miles=0.2447
Layer=1
Category=2
Driver=sqlite
Database=/home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table=t1
Key_column=cat_
Layer=1
Category=1
Driver=sqlite
Database=/home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table=t1
Key_column=cat_
cat_=1
text=xxx
number=6
Layer=2
Category=3
Driver=sqlite
Database=/home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table=t2
Key_column=cat_
Layer=2
Category=4
Driver=sqlite
Database=/home/anna/grassdata/nc_spm_08_grass7_test/vector_what/sqlite/sqlite.db
Table=t2
Key_column=cat_
cat_=4
text=yyy
number=8.09
"""


class TestMultiLayerMap(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.runModule('v.in.ascii', input='./data/testing.ascii', output='test_vector',
                      format='standard')
        cls.runModule('db.in.ogr', dsn='./data/table1.csv', output='t1')
        cls.runModule('db.in.ogr', dsn='./data/table2.csv', output='t2')
        cls.runModule('v.db.connect', map='test_vector', table='t1', key='cat_', layer=1)
        cls.runModule('v.db.connect', map='test_vector', table='t2', key='cat_', layer=2)

    @classmethod
    def tearDownClass(cls):
        cls.runModule('g.remove', type='vect', names='test_vector', flags='f')

    def setUp(self):
        self.vwhat = SimpleModule('v.what', map='test_vector',
                                  coordinates=[634243, 226193], distance=10)

    def test_run(self):
        self.assertModule(self.vwhat)
        self.assertMultiLineEqual(first=out1, second=self.vwhat.outputs.stdout)

    def test_print_options(self):
        self.vwhat.flags['a'].value = True
        self.assertModule(self.vwhat)
        self.assertMultiLineEqual(first=out2, second=self.vwhat.outputs.stdout)

        self.vwhat.flags['g'].value = True
        self.assertModule(self.vwhat)
        self.assertMultiLineEqual(first=out3, second=self.vwhat.outputs.stdout)


if __name__ == '__main__':
    test()
