#!/bin/sh
# Test the info output

# We need to set a specific region in the
# @preprocess step of this test.
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

export GRASS_OVERWRITE=1

r.mapcalc expr="prec_1 = rand(0, 550)" -s
r.mapcalc expr="prec_2 = rand(0, 450)" -s

r3.mapcalc expr="prec_1 = rand(0, 120)" -s
r3.mapcalc expr="prec_2 = rand(0, 320)" -s

v.random -z output=lidar_abs_1 n=20 zmin=0 zmax=100 column=height
v.random -z output=lidar_abs_2 n=20 zmin=0 zmax=100 column=height

# The first @test
t.info -d
t.info -dg

t.create type=strds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create type=strds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"

t.create type=str3ds temporaltype=absolute output=precip_abs1 title="A test" descr="A test"
t.create type=str3ds temporaltype=absolute output=precip_abs2 title="A test" descr="A test"

t.create type=stvds temporaltype=absolute output=lidar_abs_ds1 title="A test" descr="A test"
t.create type=stvds temporaltype=absolute output=lidar_abs_ds2 title="A test" descr="A test"

LC="fr_BE@UTF-8" t.create output=pluies_nc semantictype=sum title="précipitation_mois" description="Précipitation totale mensuelle NC"

t.register type=raster -i input=precip_abs1 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=strds input=precip_abs1
t.info -g type=strds input=precip_abs1

t.register type=raster -i input=precip_abs2 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=strds input=precip_abs2
t.info -g type=strds input=precip_abs2
t.info -h type=strds input=precip_abs2
t.info type=raster input=prec_1
t.info -g type=raster input=prec_1
t.info type=raster input=prec_2
t.info -g type=raster input=prec_2


t.register type=raster_3d -i input=precip_abs1 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=str3ds input=precip_abs1
t.info -g type=str3ds input=precip_abs1

t.register type=raster_3d -i input=precip_abs2 maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=str3ds input=precip_abs2
t.info -g type=str3ds input=precip_abs2
t.info -h type=str3ds input=precip_abs2
t.info type=raster_3d input=prec_1
t.info -g type=raster_3d input=prec_1
t.info type=raster_3d input=prec_2
t.info -g type=raster_3d input=prec_2

t.register type=vector --v -i input=lidar_abs_ds1 maps=lidar_abs_1,lidar_abs_2 start="2001-01-01" increment="20 years"
t.info type=stvds input=lidar_abs_ds1
t.info -g type=stvds input=lidar_abs_ds1

t.register type=vector --v -i input=lidar_abs_ds2 maps=lidar_abs_1,lidar_abs_2 start="2001-01-01" increment="20 years"
t.info type=stvds input=lidar_abs_ds2
t.info -g type=stvds input=lidar_abs_ds2
t.info type=vector input=lidar_abs_1
t.info -g type=vector input=lidar_abs_1
t.info type=vector input=lidar_abs_2
t.info -g type=vector input=lidar_abs_2


t.register type=raster -i input=pluies_nc maps=prec_1,prec_2 start="2001-01-01" increment="20 years"
t.info type=strds input=pluies_nc

t.unregister type=vector maps=lidar_abs_1,lidar_abs_2
t.remove type=stvds input=lidar_abs_ds1,lidar_abs_ds2

t.unregister type=raster maps=prec_1,prec_2,pluies_nc
t.remove type=strds input=precip_abs1,precip_abs2

t.unregister type=raster_3d maps=prec_1,prec_2
t.remove type=str3ds input=precip_abs1,precip_abs2

