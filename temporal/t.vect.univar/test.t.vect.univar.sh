#!/bin/sh
# Test the univar statistics of space time vector datasets

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# Data generation
v.random --o -z output=rpoints1 zmin=0 zmax=100 seed=1 column=height n=100 
v.random --o -z output=rpoints2 zmin=0 zmax=100 seed=2 column=height n=100 
v.voronoi --o input=rpoints1 output=rvoronoi1
v.voronoi --o input=rpoints2 output=rvoronoi2

t.create --o type=stvds temporaltype=absolute output=random_data title="A test" descr="A test"
t.register type=vect --v -i input=random_data maps=rpoints1,rpoints2,rvoronoi1,rvoronoi2 start="2001-01-15 12:05:45" increment="14 days"

# The first @test
t.vect.univar -h input=random_data column=height where='height > 50' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar -he input=random_data column=height where='height > 30' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar -he type=area input=random_data column=height where='height > 10' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar -he type=centroid input=random_data column=height where='height > 20' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar -h type=line input=random_data column=height where='height > 20' twhere="start_time > '2000-01-01'" layer=1

# @postprocess
t.unregister type=vect maps=rpoints1,rpoints2,rvoronoi1,rvoronoi2
t.remove type=stvds input=random_data
g.remove vect=rpoints1,rpoints2,rvoronoi1,rvoronoi2

