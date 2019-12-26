# -*- coding: utf-8 -*-
# Author: Anna Petrasova

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


# v.what map=schools,roadsmajor,elev_points,geology layer=-1,-1,-1,-1 coordinates=636661,226489 distance=1000
out1 = u"""East: 636661
North: 226489
------------------------------------------------------------------
Map: schools 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: roadsmajor 
Mapset: PERMANENT
Type: Line
Id: 231
Length: 2321.407296
Layer: 1
Category: 231
------------------------------------------------------------------
Map: elev_points 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: geology 
Mapset: PERMANENT
Type: Area
Sq Meters: 261215323.454
Hectares: 26121.532
Acres: 64547.712
Sq Miles: 100.8558
Layer: 1
Category: 217
"""

# v.what map=schools,roadsmajor,elev_points,geology layer=-1,-1,-1,-1 coordinates=636661,226489 distance=1000 -ag
out2 = u"""East: 636661
North: 226489
------------------------------------------------------------------
Map: schools 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: roadsmajor 
Mapset: PERMANENT
Type: Line
Id: 231
Length: 2321.407296
Layer: 1
Category: 231

Driver: ...
Database: ...
Table: roadsmajor
Key column: cat
cat : 231
MAJORRDS_ : 233
ROAD_NAME : I-440
MULTILANE : yes
PROPYEAR : 2015
OBJECTID : 231
SHAPE_LEN : 7616.150435
------------------------------------------------------------------
Map: elev_points 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: geology 
Mapset: PERMANENT
Type: Area
Sq Meters: 261215323.454
Hectares: 26121.532
Acres: 64547.712
Sq Miles: 100.8558
Layer: 1
Category: 217

Driver: ...
Database: ...
Table: geology
Key column: cat
cat : 217
onemap_pro : 261215328
PERIMETER : 198808.71875
GEOL250_ : 218
GEOL250_ID : 217
GEO_NAME : CZfg
SHAPE_area : 261215323.454
SHAPE_len : 198808.723525
"""

# v.what map=schools,roadsmajor,elev_points,geology layer=-1,-1,-1,-1 coordinates=636661,226489 distance=1000 -ag
out3 = u"""East=636661
North=226489

Map=schools
Mapset=PERMANENT

Map=roadsmajor
Mapset=PERMANENT
Type=Line
Id=231
Length=2321.407296
Layer=1
Category=231
Driver=...
Database=...
Table=roadsmajor
Key_column=cat
cat=231
MAJORRDS_=233
ROAD_NAME=I-440
MULTILANE=yes
PROPYEAR=2015
OBJECTID=231
SHAPE_LEN=7616.150435

Map=elev_points
Mapset=PERMANENT

Map=geology
Mapset=PERMANENT
Type=Area
Sq_Meters=261215323.454
Hectares=26121.532
Acres=64547.712
Sq_Miles=100.8558
Layer=1
Category=217
Driver=...
Database=...
Table=geology
Key_column=cat
cat=217
onemap_pro=261215328
PERIMETER=198808.71875
GEOL250_=218
GEOL250_ID=217
GEO_NAME=CZfg
SHAPE_area=261215323.454
SHAPE_len=198808.723525
"""

# v.what map=schools,roadsmajor,elev_points,geology layer=-1,-1,-1,-1 coordinates=636661,226489 distance=100
out4 = u"""East: 636661
North: 226489
------------------------------------------------------------------
Map: schools 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: roadsmajor 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: elev_points 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: geology 
Mapset: PERMANENT
Type: Area
Sq Meters: 261215323.454
Hectares: 26121.532
Acres: 64547.712
Sq Miles: 100.8558
Layer: 1
Category: 217
"""


class TestNCMaps(TestCase):

    def setUp(self):
        self.vwhat = SimpleModule('v.what', map=['schools', 'roadsmajor', 'elev_points', 'geology'],
                                  layer=['-1', '-1', '-1', '-1'], coordinates=[636661, 226489],
                                  distance=1000)

    def test_multiple_maps(self):
        self.assertModule(self.vwhat)
        self.assertMultiLineEqual(first=out1, second=self.vwhat.outputs.stdout)

    def test_print_options(self):
        self.vwhat.flags['a'].value = True
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out2, actual=self.vwhat.outputs.stdout)

        self.vwhat.flags['g'].value = True
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out3, actual=self.vwhat.outputs.stdout)

    def test_threshold(self):
        self.vwhat.inputs['distance'].value = 100
        self.assertModule(self.vwhat)
        self.assertLooksLike(reference=out4, actual=self.vwhat.outputs.stdout)

    def test_print_options_json(self):
        import json
        self.vwhat.flags['j'].value = True
        self.assertModule(self.vwhat)
        try:
            json.loads(self.vwhat.outputs.stdout)
        except ValueError:
            self.fail(msg="No JSON object could be decoded:\n" + self.vwhat.outputs.stdout)

if __name__ == '__main__':
    test()
