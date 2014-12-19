#!/bin/sh
# This is a test to register and unregister raster3d maps in
# space time raster3d datasets
# The raster3d maps will be registered in different space time raster3d
# datasets

# We need to set a specific region in the
# @preprocess step of this test. We generate
# 3d raster with r3.mapcalc and create two space time raster3d datasets
# with absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

# Generate data
r3.mapcalc --o expr="volume_1 = rand(0, 550)" -s
r3.mapcalc --o expr="volume_2 = rand(0, 450)" -s
r3.mapcalc --o expr="volume_3 = rand(0, 320)" -s
r3.mapcalc --o expr="volume_4 = rand(0, 510)" -s
r3.mapcalc --o expr="volume_5 = rand(0, 300)" -s
r3.mapcalc --o expr="volume_6 = rand(0, 650)" -s

# The first @test
# We create the space time raster3d dataset and register the raster3d maps with absolute time interval

t.create --v --o type=str3ds temporaltype=absolute output=volume_abs1 title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs2 title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs3 title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs4 title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs5 title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs6 title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs7 title="A test" descr="A test"

t.register type=raster_3d --o --v -i input=volume_abs1 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="1 seconds"
t.info type=str3ds input=volume_abs1
t.unregister --v type=raster_3d input=volume_abs1 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6
t.info type=str3ds input=volume_abs1

t.register type=raster_3d --o --v -i input=volume_abs2 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="20 seconds, 5 minutes"
t.info type=str3ds input=volume_abs2
r3.info volume_1
r3.info volume_2
r3.info volume_3
r3.info volume_4
r3.info volume_5
r3.info volume_6

t.register type=raster_3d --o --v -i input=volume_abs3 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="8 hours"
t.info type=str3ds input=volume_abs3
t.unregister --v type=raster_3d maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6
t.info type=str3ds input=volume_abs3

t.register type=raster_3d --o input=volume_abs4 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="3 days"
t.info type=str3ds input=volume_abs4

t.register type=raster_3d --o input=volume_abs5 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="4 weeks"
t.info type=str3ds input=volume_abs5

t.register type=raster_3d --o input=volume_abs6 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-08-01" increment="2 months"
t.info type=str3ds input=volume_abs6

t.register type=raster_3d --o input=volume_abs7 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="20 years, 3 months, 1 days, 4 hours"
t.info type=str3ds input=volume_abs7
# Register with different valid time again
t.register type=raster_3d --o input=volume_abs7 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=str3ds input=volume_abs7
# Register with different valid time again creating intervals
t.register type=raster_3d -i --o input=volume_abs7 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=str3ds input=volume_abs7

t.unregister --v type=raster_3d maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6
t.remove --v type=str3ds input=volume_abs1,volume_abs2,volume_abs3,volume_abs4,volume_abs5,volume_abs6,volume_abs7
