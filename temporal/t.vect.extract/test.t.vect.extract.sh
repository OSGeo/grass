#!/bin/sh
# Test the extraction of a subset of a space time vector input

# We need to set a specific region in the
# @preprocess step of this test.

v.random --o -z output=soil_1 n=100 zmin=0 zmax=100 column=height  seed=1
v.random --o -z output=soil_2 n=100 zmin=0 zmax=100 column=height seed=2
v.random --o -z output=soil_3 n=100 zmin=0 zmax=100 column=height seed=3
v.random --o -z output=soil_4 n=100 zmin=0 zmax=100 column=height seed=4
v.random --o -z output=soil_5 n=100 zmin=0 zmax=100 column=height seed=5
v.random --o -z output=soil_6 n=100 zmin=0 zmax=100 column=height seed=6
v.random --o -z output=soil_7 n=100 zmin=0 zmax=100 column=height seed=7
v.random --o -z output=soil_8 n=100 zmin=0 zmax=100 column=height seed=8

n1=`g.tempfile pid=1 -d` 

cat > "${n1}" << EOF
soil_1
soil_2
soil_3
soil_4
soil_5
soil_6
soil_7
soil_8
EOF

t.create --o type=stvds temporaltype=absolute output=soil_abs1 title="A test" descr="A test"
t.register -i type=vector input=soil_abs1 file="${n1}" start='2001-01-01' increment="1 months"
t.info type=stvds input=soil_abs1
t.vect.list input=soil_abs1

# The @test
t.vect.extract --v input=soil_abs1 output=soil_abs2 where="start_time > '2001-03-01 00:00:01'"
t.info type=stvds input=soil_abs2
t.vect.list input=soil_abs2

t.vect.extract --v --o input=soil_abs1 output=soil_abs3 where="start_time >= '2001-01-01'" \
               base=new_vect expr=" height > 50" nprocs=1
t.info type=stvds input=soil_abs3
t.vect.list input=soil_abs3 columns=name,start_time,end_time,primitives

# @postprocess
t.unregister type=vector maps=soil_1,soil_2,soil_3,soil_4,soil_5,soil_6,soil_7,soil_8
t.remove type=stvds input=soil_abs1,soil_abs2,soil_abs3
g.remove -f type=vector name=soil_1,soil_2,soil_3,soil_4,soil_5,soil_6,soil_7,soil_8
g.remove -f type=vector name=new_vect_1,new_vect_2,new_vect_3,new_vect_4,new_vect_5,new_vect_6
