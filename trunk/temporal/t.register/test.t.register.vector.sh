#!/bin/sh
# This is a test to register and unregister vector maps in
# space time vector input.

# We need to set a specific region in the
# @preprocess step of this test. We generate
# vector with v.random and create several space time vector inputs
# with absolute time
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

v.random --o -z output=lidar_abs_1 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_abs_2 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_abs_3 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_abs_4 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_abs_5 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_abs_6 n=20 zmin=0 zmax=100 column=height

# The first @test
# We create the space time vector inputs and register the vector maps with absolute time interval

t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds1 title="A test" descr="A test"
t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds2 title="A test" descr="A test"
t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds3 title="A test" descr="A test"
t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds4 title="A test" descr="A test"
t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds5 title="A test" descr="A test"
t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds6 title="A test" descr="A test"
t.create --v --o type=stvds temporaltype=absolute output=lidar_abs_ds7 title="A test" descr="A test"

t.register type=vector --o --v -i input=lidar_abs_ds1 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="1 seconds"
t.info type=stvds input=lidar_abs_ds1
t.unregister --v type=vector input=lidar_abs_ds1 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6
t.info type=stvds input=lidar_abs_ds1

t.register type=vector --o --v -i input=lidar_abs_ds2 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="20 seconds, 5 minutes"
t.info type=stvds input=lidar_abs_ds2

t.register type=vector --o --v -i input=lidar_abs_ds3 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="8 hours"
t.info type=stvds input=lidar_abs_ds3
t.unregister --v type=vector maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6
t.info type=stvds input=lidar_abs_ds3

t.register type=vector --o input=lidar_abs_ds4 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="3 days"
t.info type=stvds input=lidar_abs_ds4

t.register type=vector --o input=lidar_abs_ds5 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="4 weeks"
t.info type=stvds input=lidar_abs_ds5

t.register type=vector --o input=lidar_abs_ds6 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-08-01" increment="2 months"
t.info type=stvds input=lidar_abs_ds6

t.register type=vector --o input=lidar_abs_ds7 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="20 years, 3 months, 1 days, 4 hours"
t.info type=stvds input=lidar_abs_ds7
# Register with different valid time again
t.register type=vector --o input=lidar_abs_ds7 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=stvds input=lidar_abs_ds7
# Register with different valid time again creating an interval
t.register type=vector --o -i input=lidar_abs_ds7 maps=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6 start="2001-01-01" increment="99 years, 9 months, 9 days, 9 hours"
t.info type=stvds input=lidar_abs_ds7

t.unregister --v type=vector maps=lidar_abs_1,lidar_abs_2,lidar_abs_3
# Test warning
t.unregister --v type=vector maps=lidar_abs_1,lidar_abs_2,lidar_abs_3
t.remove --v type=stvds input=lidar_abs_ds1,lidar_abs_ds2,lidar_abs_ds3,lidar_abs_ds4,lidar_abs_ds5,lidar_abs_ds6,lidar_abs_ds7
t.unregister --v type=vector maps=lidar_abs_4,lidar_abs_5,lidar_abs_6
g.remove -f type=vector name=lidar_abs_1,lidar_abs_2,lidar_abs_3,lidar_abs_4,lidar_abs_5,lidar_abs_6


