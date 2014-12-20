# This script tests r.pack and r.unpack

# We specify a small region in the @preprocess step
# The region setting should work for UTM and LL test locations
g.region s=0 n=70 w=0 e=100 b=0 t=50 -p

# Map generation
r.mapcalc --o expr="test_int = 1"
r.mapcalc --o expr="test_float = float(2.0)"
r.mapcalc --o expr="test_double = double(3.0)"

# @test packing and unpacking with @precision=1 
# First we pack the different raster maps with and without compression

r.pack --o --v input=test_int
r.pack --o --v input=test_float
r.pack --o --v input=test_double

r.pack --o --v -c input=test_int    output=test_int_uncompressed.pack
r.pack --o --v -c input=test_float  output=test_float_uncompressed.pack
r.pack --o --v -c input=test_double output=test_double_uncompressed.pack

# Remove generated maps and re-import
g.remove -f type=raster name=test_int,test_float,test_double
# Second we check the import with r.unpack

r.unpack --o --v input=test_int.pack
r.unpack --o --v input=test_float.pack
r.unpack --o --v input=test_double.pack

# Generating reference data
#r.out.ascii --o input=test_int output=test_int.ref dp=1
#r.out.ascii --o input=test_float output=test_float.ref dp=1
#r.out.ascii --o input=test_double output=test_double.ref dp=1

r.unpack -o --v input=test_int_uncompressed.pack    output=test_int_uncompressed
r.unpack -o --v input=test_float_uncompressed.pack  output=test_float_uncompressed
r.unpack -o --v -o input=test_double_uncompressed.pack output=test_double_uncompressed

# Generating reference data
#r.out.ascii --o input=test_int_uncompressed output=test_int_uncompressed.ref dp=1
#r.out.ascii --o input=test_float_uncompressed output=test_float_uncompressed.ref dp=1
#r.out.ascii --o input=test_double_uncompressed output=test_double_uncompressed.ref dp=1

g.remove -f type=raster name=test_int,test_float,test_double,test_int_uncompressed,test_float_uncompressed,test_double_uncompressed
rm *.pack
