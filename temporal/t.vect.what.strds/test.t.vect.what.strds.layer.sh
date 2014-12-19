#!/bin/sh
# Test the temporal and spatial sampling of raster maps by vector point maps
# using timetamped vector tables
# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# The raster maps
r.mapcalc --o expr="sand_frac   = 50.0"
r.mapcalc --o expr="sand_frac_1 = 25.0"
r.mapcalc --o expr="sand_frac_2 = 35.0"
r.mapcalc --o expr="sand_frac_3 = 45.0"
r.mapcalc --o expr="sand_frac_4 = 55.0"
r.mapcalc --o expr="sand_frac_5 = 65.0"
r.mapcalc --o expr="sand_frac_6 = 75.0"
# The vector map
v.random --o -z seed=1 output=soil_orig n=100 zmin=0 zmax=100 column=sand
# Adding new layer with categories
v.category input=soil_orig out=soils option=transfer layer=1,2,3,4,5,6 --o
# Creating new tables for each layer
db.copy from_table=soil_orig to_table=soils_2
db.copy from_table=soil_orig to_table=soils_3
db.copy from_table=soil_orig to_table=soils_4
db.copy from_table=soil_orig to_table=soils_5
db.copy from_table=soil_orig to_table=soils_6
v.db.addtable map=soils table=soils_2 layer=2 
v.db.addtable map=soils table=soils_3 layer=3 
v.db.addtable map=soils table=soils_4 layer=4 
v.db.addtable map=soils table=soils_5 layer=5 
v.db.addtable map=soils table=soils_6 layer=6 

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
soils:1
soils:2
soils:3
soils:4
soils:5
soils:6
EOF

t.create --o type=stvds temporaltype=absolute output=soil_abs title="A test" descr="A test"
t.register -i type=vector input=soil_abs file="${n1}" start="2001-03-01 00:00:00" increment="1 months"
t.vect.list input=soil_abs columns=name,layer,start_time,end_time

t.create --o type=strds temporaltype=absolute output=sand_frac_abs_1 title="A test" descr="A test"
t.register -i type=raster input=sand_frac_abs_1 maps=sand_frac start="2001-01-01 00:00:00" increment="12 months"
t.rast.list input=sand_frac_abs_1 columns=name,start_time,end_time

t.create --o type=strds temporaltype=absolute output=sand_frac_abs_2 title="A test" descr="A test"
t.register -i type=raster input=sand_frac_abs_2 maps=sand_frac_1,sand_frac_2,sand_frac_3,sand_frac_4,sand_frac_5,sand_frac_6 \
            start="2001-03-01 00:00:00" increment="1 months"
t.rast.list input=sand_frac_abs_2 columns=name,start_time,end_time

# Start the @test
t.vect.what.strds --v input=soil_abs strds=sand_frac_abs_1 sampling=overlap,during,contain column=sand_frac
v.db.select map=soils layer=1
v.db.select map=soils layer=2
v.db.select map=soils layer=3
v.db.select map=soils layer=4
v.db.select map=soils layer=5
v.db.select map=soils layer=6

t.vect.what.strds --v input=soil_abs strds=sand_frac_abs_2 sampling=equal
v.db.select map=soils layer=1
v.db.select map=soils layer=2
v.db.select map=soils layer=3
v.db.select map=soils layer=4
v.db.select map=soils layer=5
v.db.select map=soils layer=6

# @postprocess
t.unregister type=vector maps=soils:1,soils:2,soils:3,soils:4,soils:5,soils:6
t.remove type=stvds input=soil_abs

t.unregister type=raster maps=sand_frac,sand_frac_1,sand_frac_2,sand_frac_3,sand_frac_4,sand_frac_5,sand_frac_6
t.remove type=strds input=sand_frac_abs_1,sand_frac_abs_2

g.remove -f type=raster name=sand_frac,sand_frac_1,sand_frac_2,sand_frac_3,sand_frac_4,sand_frac_5,sand_frac_6
g.remove -f type=vector name=soils,soil_orig
