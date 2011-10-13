# This is a test to register and unregister raster3d maps in
# space time raster3d input.
# The raster3d maps will be registered in different space time raster3d
# inputs

# We need to set a specific region in the
# @preprocess step of this test. We generate
# 3d raster with r3.mapcalc and create two space time raster3d inputs
# with relative and absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r3.mapcalc --o expr="volume_1 = rand(0, 550)"
r3.mapcalc --o expr="volume_2 = rand(0, 450)"
r3.mapcalc --o expr="volume_3 = rand(0, 320)"
r3.mapcalc --o expr="volume_4 = rand(0, 510)"
r3.mapcalc --o expr="volume_5 = rand(0, 300)"
r3.mapcalc --o expr="volume_6 = rand(0, 650)"

# The first @test
# We create the space time raster3d inputs and register the raster3d maps with absolute time interval

t.create --v --o type=str3ds temporaltype=absolute output=volume_abs1 gran="1 senconds" title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs2 gran="1 minutes" title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs3 gran="1 hours" title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs4 gran="1 days" title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs5 gran="1 weeks" title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs6 gran="1 months" title="A test" descr="A test"
t.create --v --o type=str3ds temporaltype=absolute output=volume_abs7 gran="1 years" title="A test" descr="A test"

tr3.register --v -i input=volume_abs1 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="1 seconds"
t.info type=str3ds input=volume_abs1
tr3.unregister --v input=volume_abs1 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6
t.info type=str3ds input=volume_abs1

tr3.register --v -i input=volume_abs2 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="20 seconds, 5 minutes"
t.info type=str3ds input=volume_abs2

tr3.register --v -i input=volume_abs3 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="8 hours"
t.info type=str3ds input=volume_abs3
tr3.unregister --v maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6
t.info type=str3ds input=volume_abs3

tr3.register input=volume_abs4 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="3 days"
t.info type=str3ds input=volume_abs4

tr3.register input=volume_abs5 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="4 weeks"
t.info type=str3ds input=volume_abs5

tr3.register input=volume_abs6 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-08-01" increment="2 months"
t.info type=str3ds input=volume_abs6

tr3.register input=volume_abs7 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="20 years, 3 months, 1 days, 4 hours"
t.info type=str3ds input=volume_abs7
# Register with different valid time again
tr3.register input=volume_abs7 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=str3ds input=volume_abs7
# Register with different valid time again creating intervals
tr3.register -i input=volume_abs7 maps=volume_1,volume_2,volume_3,volume_4,volume_5,volume_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=str3ds input=volume_abs7

t.remove --v type=rast3d input=volume_1,volume_2,volume_3
t.remove --v type=str3ds input=volume_abs1,volume_abs2,volume_abs3,volume_abs4,volume_abs5,volume_abs6,volume_abs7
t.remove --v type=rast3d input=volume_4,volume_5,volume_6
