#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Nov 14 09:43:55 2017

@author: lucadelu
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command

output="""table:zipcodes
description:
insert:?
delete:?
ncols:12
nrows:44

column:cat
description:
type:INTEGER
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:OBJECTID
description:
type:INTEGER
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:WAKE_ZIPCO
description:
type:DOUBLE PRECISION
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:PERIMETER
description:
type:DOUBLE PRECISION
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:ZIPCODE_
description:
type:DOUBLE PRECISION
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:ZIPCODE_ID
description:
type:DOUBLE PRECISION
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:ZIPNAME
description:
type:CHARACTER
len:15
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:ZIPNUM
description:
type:DOUBLE PRECISION
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:ZIPCODE
description:
type:CHARACTER
len:30
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:NAME
description:
type:CHARACTER
len:30
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:SHAPE_Leng
description:
type:DOUBLE PRECISION
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?

column:SHAPE_Area
description:
type:DOUBLE PRECISION
len:20
scale:0
precision:0
default:
nullok:yes
select:?
update:?
"""

outcol="""ncols: 12
nrows: 44
Column 1: cat:INTEGER:20
Column 2: OBJECTID:INTEGER:20
Column 3: WAKE_ZIPCO:DOUBLE PRECISION:20
Column 4: PERIMETER:DOUBLE PRECISION:20
Column 5: ZIPCODE_:DOUBLE PRECISION:20
Column 6: ZIPCODE_ID:DOUBLE PRECISION:20
Column 7: ZIPNAME:CHARACTER:15
Column 8: ZIPNUM:DOUBLE PRECISION:20
Column 9: ZIPCODE:CHARACTER:30
Column 10: NAME:CHARACTER:30
Column 11: SHAPE_Leng:DOUBLE PRECISION:20
Column 12: SHAPE_Area:DOUBLE PRECISION:20
"""


class TestDbCopy(TestCase):
    invect = 'zipcodes'
    mapset = '$GISDBASE/$LOCATION_NAME/PERMANENT/sqlite/sqlite.db'

    @classmethod
    def setUpClass(cls):
        cls.runModule('db.connect', flags='c')

    def test_describe(self):
        cols = read_command('db.describe', table=self.invect,
                            database=self.mapset)
        self.assertEqual(first=cols, second=output)

    def test_columns(self):
        cols = read_command('db.describe', table=self.invect, flags='c',
                            database=self.mapset)
        self.assertEqual(first=cols, second=outcol)


if __name__ == '__main__':
    test()
