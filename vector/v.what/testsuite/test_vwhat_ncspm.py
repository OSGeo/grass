# -*- coding: utf-8 -*-
from grass.gunittest import TestCase, test
from grass.gunittest.gmodules import SimpleModule


out1 = """East: 636661
North: 226489
------------------------------------------------------------------
Map: bridges 
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
Map: precip_30ynormals_3d 
Mapset: PERMANENT
Type: Point
Id: 96
Point height: 121.920000
Layer: 1
Category: 96
------------------------------------------------------------------
Map: lakes 
Mapset: PERMANENT
Type: Area
Sq Meters: 9397.338
Hectares: 0.940
Acres: 2.322
Sq Miles: 0.0036
Layer: 1
Category: 7168
"""

out2 = """East: 636661
North: 226489
------------------------------------------------------------------
Map: bridges 
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
Map: precip_30ynormals_3d 
Mapset: PERMANENT
Type: Point
Id: 96
Point height: 121.920000
Layer: 1
Category: 96

Driver: ...
Database: ...
Table: precip_30ynormals_3d
Key column: cat
cat : 96
station : 317079
lat : 35.79444
long : -78.69889
elev : 121.92
jan : 113.284
feb : 89.662
mar : 113.284
apr : 75.692
may : 102.362
jun : 103.124
jul : 110.49
aug : 109.22
sep : 108.458
oct : 96.012
nov : 77.724
dec : 81.534
annual : 1178.56
------------------------------------------------------------------
Map: lakes 
Mapset: PERMANENT
Type: Area
Sq Meters: 9397.338
Hectares: 0.940
Acres: 2.322
Sq Miles: 0.0036
Layer: 1
Category: 7168

Driver: ...
Database: ...
Table: lakes
Key column: cat
cat : 7168
AREA : 101151.70356
PERIMETER : 1549.92239
FULL_HYDRO : 7169
FULL_HYDR2 : 157379
FTYPE : LAKE/POND
FCODE : 39000
NAME : 
"""

out3 = """East=636661
North=226489

Map=bridges
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

Map=precip_30ynormals_3d
Mapset=PERMANENT
Type=Point
Id=96
Point_height=121.920000
Layer=1
Category=96
Driver=...
Database=...
Table=precip_30ynormals_3d
Key_column=cat
cat=96
station=317079
lat=35.79444
long=-78.69889
elev=121.92
jan=113.284
feb=89.662
mar=113.284
apr=75.692
may=102.362
jun=103.124
jul=110.49
aug=109.22
sep=108.458
oct=96.012
nov=77.724
dec=81.534
annual=1178.56

Map=lakes
Mapset=PERMANENT
Type=Area
Sq_Meters=9397.338
Hectares=0.940
Acres=2.322
Sq_Miles=0.0036
Layer=1
Category=7168
Driver=...
Database=...
Table=lakes
Key_column=cat
cat=7168
AREA=101151.70356
PERIMETER=1549.92239
FULL_HYDRO=7169
FULL_HYDR2=157379
FTYPE=LAKE/POND
FCODE=39000
NAME=
"""

out4 = """East: 636661
North: 226489
------------------------------------------------------------------
Map: bridges 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: roadsmajor 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: precip_30ynormals_3d 
Mapset: PERMANENT
Nothing found.
------------------------------------------------------------------
Map: lakes 
Mapset: PERMANENT
Type: Area
Sq Meters: 9397.338
Hectares: 0.940
Acres: 2.322
Sq Miles: 0.0036
Layer: 1
Category: 7168
"""


class TestNCMaps(TestCase):

    def setUp(self):
        self.vwhat = SimpleModule('v.what', map=['bridges', 'roadsmajor', 'precip_30ynormals_3d', 'lakes'],
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
