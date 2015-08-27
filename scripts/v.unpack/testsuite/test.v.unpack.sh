# This script tests v.pack and v.unpack

# We specific a small region in the
# @preprocess step
# The region setting should work for UTM and LL test locations
g.region s=0 n=70 w=0 e=100 b=0 t=50 -p

# We need different vector maps, with and without tables 
# and with multiple layers
v.random --o -z output=probe_1 n=100 zmin=0 zmax=100 seed=1
v.random --o -z output=probe_2 n=100 zmin=0 zmax=100 column=height seed=1
v.random --o -z output=probe_orig n=100 zmin=0 zmax=100 column=height seed=1
# Adding new layer with categories
v.category input=probe_orig out=probe_3 option=transfer layer=1,2,3 --o

# Creating new tables for each layer
db.copy from_table=probe_orig to_table=probe_3_1
db.copy from_table=probe_orig to_table=probe_3_2
db.copy from_table=probe_orig to_table=probe_3_3

# Removing un-needed vectors and tables
g.remove -f type=vector name=probe_orig
v.db.droptable -f map=probe_3 table=probe_3 layer=1

# Adding tables to layer
v.db.addtable --o map=probe_3 table=probe_3_1 layer=1 
v.db.addtable --o map=probe_3 table=probe_3_2 layer=2 
v.db.addtable --o map=probe_3 table=probe_3_3 layer=3 

# First we @test the packing/export with v.pack
v.pack --o input=probe_1
v.pack --o input=probe_2
v.pack --o input=probe_3

v.pack --o -c input=probe_1 output=probe_1_uncompressed.pack
v.pack --o -c input=probe_2 output=probe_2_uncompressed.pack
v.pack --o -c input=probe_3 output=probe_3_uncompressed.pack

# We need to clean before import
g.remove -f type=vector name=probe_1,probe_2,probe_3

# Test the compressed import with v.unpack
v.unpack --o input=probe_1.pack
v.category input=probe_1 option=report
v.unpack --o input=probe_2.pack
v.category input=probe_2 option=report
v.unpack --o input=probe_3.pack
v.category input=probe_3 option=report

# Test the uncompressed import with v.unpack (Seems to fail?)
v.unpack --o input=probe_1_uncompressed.pack output=probe_1_uncompressed
v.category input=probe_1_uncompressed option=report
v.unpack --o input=probe_2_uncompressed.pack output=probe_2_uncompressed
v.category input=probe_2_uncompressed option=report
v.unpack --o input=probe_3_uncompressed.pack output=probe_3_uncompressed
v.category input=probe_3_uncompressed option=report

g.remove -f type=vector name=probe_1_uncompressed,probe_2_uncompressed,probe_3_uncompressed
#rm *.pack
