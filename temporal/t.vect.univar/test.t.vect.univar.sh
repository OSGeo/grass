#!/bin/sh
# Test the univar statistics of space time vector datasets

# We need to set a specific region in the
# @preprocess step of this test. 
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

export GRASS_OVERWRITE=1

# Data generation
v.random -z output=rpoints1 zmin=0 zmax=100 seed=1 column=height n=100 
v.random -z output=rpoints2 zmin=0 zmax=100 seed=2 column=height n=100 
v.voronoi input=rpoints1 output=rvoronoi1
v.voronoi input=rpoints2 output=rvoronoi2

t.create type=stvds temporaltype=absolute output=random_data title="A test" descr="A test"
t.register type=vector --v -i input=random_data maps=rpoints1,rpoints2,rvoronoi1,rvoronoi2 start="2001-01-15 12:05:45" increment="14 days"

# The first @test
t.vect.univar input=random_data column=height where='height > 50' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar -e input=random_data column=height where='height > 30' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar -e type=area input=random_data column=height where='height > 10' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar -e type=centroid input=random_data column=height where='height > 20' twhere="start_time > '2000-01-01'" layer=1
t.vect.univar type=line input=random_data column=height where='height > 20' twhere="start_time > '2000-01-01'" layer=1

# @postprocess
t.unregister type=vector maps=rpoints1,rpoints2,rvoronoi1,rvoronoi2
t.remove type=stvds input=random_data
g.remove -f type=vector name=rpoints1,rpoints2,rvoronoi1,rvoronoi2

