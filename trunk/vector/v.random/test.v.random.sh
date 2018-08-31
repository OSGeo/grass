# This is a simple test for v.random
# We create several identical pseudo random points maps :)
# using the seed option for rand and drand48

# In the @preprocess step we set up a suitable region
g.region n=80 s=0 w=0 e=120 res=10 -p

# First @test the rand function. Create a 3d vector map with attribute table
# The validation is based on @vector map with a @precision=3
v.random --o -z output=test_random_vect_1 n=20 zmin=0 zmax=100 seed=501
# Now the attribute @table should be validated. Booth maps are identical
v.random --o -z output=test_random_vect_2 n=20 zmin=0 zmax=100 column=height seed=501

# Second @test the drand48 function. Create a 3d vector map with attribute table
# The validation is based on @vector map with a @precision=3
v.random --o -zd output=test_random_vect_3 n=20 zmin=0 zmax=100 seed=501
# Now the attribute @table should be validated. Booth maps are identical
v.random --o -zd output=test_random_vect_4 n=20 zmin=0 zmax=100 column=height seed=501

# Export the generated data as references
# v.out.ascii --o format=point dp=3 input=test_random_vect_1 output=test_random_vect_1.ref
# db.select "select * from test_random_vect_2" > test_random_vect_2.ref
# v.out.ascii --o format=point dp=3 input=test_random_vect_3 output=test_random_vect_3.ref
# db.select "select * from test_random_vect_4" > test_random_vect_4.ref