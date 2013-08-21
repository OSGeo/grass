#!/bin/sh
# Test the info output

# We need to set a specific region in the
# @preprocess step of this test. 
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

r.mapcalc --o expr="prec_1 = rand(0, 550)"
r.mapcalc --o expr="prec_2 = rand(0, 450)"

r3.mapcalc --o expr="prec_1 = rand(0, 120)"
r3.mapcalc --o expr="prec_2 = rand(0, 320)"

v.random --o -z output=lidar_abs_1 n=20 zmin=0 zmax=100 column=height
v.random --o -z output=lidar_abs_2 n=20 zmin=0 zmax=100 column=height

# The first @test
t.info -s
t.info -sg

t.create --o type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create --o type=strds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"

t.create --o type=str3ds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create --o type=str3ds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"

t.create --o type=stvds temporaltype=absolute output=lidar_abs_ds1 title="A test" descr="A test"
t.create --o type=stvds temporaltype=absolute output=lidar_abs_ds2 title="A test" descr="A test"

t.register --o type=rast -i input=precip_abs1 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=strds input=precip_abs1
t.info -g type=strds input=precip_abs1

t.register --o type=rast -i input=precip_abs2 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=strds input=precip_abs2
t.info -g type=strds input=precip_abs2
t.info -h type=strds input=precip_abs2
t.info type=rast input=prec_1
t.info -g type=rast input=prec_1
t.info type=rast input=prec_2
t.info -g type=rast input=prec_2


t.register --o type=rast3d -i input=precip_abs1 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=str3ds input=precip_abs1
t.info -g type=str3ds input=precip_abs1

t.register --o type=rast3d -i input=precip_abs2 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=str3ds input=precip_abs2
t.info -g type=str3ds input=precip_abs2
t.info -h type=str3ds input=precip_abs2
t.info type=rast3 input=prec_1
t.info -g type=rast3 input=prec_1
t.info type=rast3 input=prec_2
t.info -g type=rast3 input=prec_2

t.register --o type=vect --v -i input=lidar_abs_ds1 maps=lidar_abs_1,lidar_abs_2 start="2001-01-01" increment="20 years"
t.info type=stvds input=lidar_abs_ds1
t.info -g type=stvds input=lidar_abs_ds1

t.register --o type=vect --v -i input=lidar_abs_ds2 maps=lidar_abs_1,lidar_abs_2 start="2001-01-01" increment="20 years"
t.info type=stvds input=lidar_abs_ds2
t.info -g type=stvds input=lidar_abs_ds2
t.info type=vect input=lidar_abs_1
t.info -g type=vect input=lidar_abs_1
t.info type=vect input=lidar_abs_2
t.info -g type=vect input=lidar_abs_2

t.unregister type=vect maps=lidar_abs_1,lidar_abs_2
t.remove type=stvds input=lidar_abs_ds1,lidar_abs_ds2

t.unregister type=rast maps=prec_1,prec_2
t.remove type=strds input=precip_abs1,precip_abs2

t.unregister type=rast3 maps=prec_1,prec_2
t.remove type=str3ds input=precip_abs1,precip_abs2

