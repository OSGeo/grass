# -*- coding: utf-8 -*-
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

out1 = u"""East: 634243
North: 226193
------------------------------------------------------------------
Map: test_vector 
Mapset: ...
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


out2 = u"""East: 634243
North: 226193
------------------------------------------------------------------
Map: test_vector 
Mapset: ...
Type: Area
Sq Meters: 633834.281
Hectares: 63.383
Acres: 156.624
Sq Miles: 0.2447
Layer: 1
Category: 2

Driver: ...
Database: ...
Table: t1
Key column: cat_
Layer: 1
Category: 1

Driver: ...
Database: ...
Table: t1
Key column: cat_
cat_ : 1
text : Petrášová
number : 6
Layer: 2
Category: 3

Driver: ...
Database: ...
Table: t2
Key column: cat_
Layer: 2
Category: 4

Driver: ...
Database: ...
Table: t2
Key column: cat_
cat_ : 4
text : yyy
number : 8.09
"""

out3 = u"""East=634243
North=226193

Map=test_vector
Mapset=...
Type=Area
Sq_Meters=633834.281
Hectares=63.383
Acres=156.624
Sq_Miles=0.2447
Layer=1
Category=2
Driver=...
Database=...
Table=t1
Key_column=cat_
Layer=1
Category=1
Driver=...
Database=...
Table=t1
Key_column=cat_
cat_=1
text=Petrášová
number=6
Layer=2
Category=3
Driver=...
Database=...
Table=t2
Key_column=cat_
Layer=2
Category=4
Driver=...
Database=...
Table=t2
Key_column=cat_
cat_=4
text=yyy
number=8.09
"""

out4 = u"""East=634243
North=226193

Map=test_vector
Mapset=...
Type=Area
Sq_Meters=633834.281
Hectares=63.383
Acres=156.624
Sq_Miles=0.2447
Layer=1
Category=2
Driver=...
Database=...
Table=t1
Key_column=cat_
Layer=1
Category=1
Driver=...
Database=...
Table=t1
Key_column=cat_
cat_=1
text=Petrášová
number=6
"""

out5 = u"""East=634243
North=226193

Map=test_vector
Mapset=...
Type=Area
Sq_Meters=633834.281
Hectares=63.383
Acres=156.624
Sq_Miles=0.2447
Layer=2
Category=3
Layer=2
Category=4
"""


class TestMultiLayerMap(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.runModule('v.in.ascii', input='./data/testing.ascii', output='test_vector',
                      format='standard')
        cls.runModule('db.connect', flags='c')
        cls.runModule('db.in.ogr', input='./data/table1.csv', output='t1')
        cls.runModule('db.in.ogr', input='./data/table2.csv', output='t2')
        cls.runModule('v.db.connect', map='test_vector', table='t1', key='cat_', layer=1)
        cls.runModule('v.db.connect', map='test_vector', table='t2', key='cat_', layer=2)

    @classmethod
    def tearDownClass(cls):
        cls.runModule('g.remove', type='vector', name='test_vector', flags='f')

    def setUp(self):
        self.vwhat = SimpleModule('v.what', map='test_vector',
                                  coordinates=[634243, 226193], distance=10)

    def test_run(self):
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out1, actual=self.vwhat.outputs.stdout)

    def test_print_options(self):
        self.vwhat.flags['a'].value = True
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out2, actual=self.vwhat.outputs.stdout)

        self.vwhat.flags['g'].value = True
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out3, actual=self.vwhat.outputs.stdout)

    def test_print_options_json(self):
        import json
        self.vwhat.flags['j'].value = True
        self.vwhat.flags['a'].value = True
        self.assertModule(self.vwhat)
        try:
            json.loads(self.vwhat.outputs.stdout)
        except ValueError:
            self.fail(msg="No JSON object could be decoded:\n" + self.vwhat.outputs.stdout)

    def test_selected_layers(self):
        self.vwhat.inputs.layer = -1
        self.vwhat.flags['g'].value = True
        self.vwhat.flags['a'].value = True
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out3, actual=self.vwhat.outputs.stdout)

        self.vwhat.inputs.layer = 1
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out4, actual=self.vwhat.outputs.stdout)

        self.vwhat.inputs.layer = 2
        self.vwhat.flags['a'].value = False
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out5, actual=self.vwhat.outputs.stdout)

if __name__ == '__main__':
    test()
