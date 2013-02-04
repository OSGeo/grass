#!/bin/sh

g.region w=0 e=180 s=0 n=90 b=0 t=100 res3=10

r3.mapcalc --o expr="test_out_bin_float = float(if(row() == 2, null(), row()))"
r3.mapcalc --o expr="test_out_bin_double = double(if(row() == 2, null(), row()))"
r3.out.ascii --o dp=0 input=test_out_bin_float output=test_out_bin_float.ref; 
r3.out.ascii --o dp=0 input=test_out_bin_double output=test_out_bin_double.ref; 

# @test

r3.out.bin --o input=test_out_bin_float byte=4 null=-9999 \
    output=test_out_bin_float_native_b4.bin order=native

r3.out.bin --o input=test_out_bin_float byte=8 null=-9999 \
    output=test_out_bin_float_native_b8.bin order=native

r3.out.bin --o -r input=test_out_bin_float byte=4 null=-9999 \
    output=test_out_bin_float_native_r_b4.bin order=native

r3.out.bin --o -d input=test_out_bin_float byte=4 null=-9999 \
    output=test_out_bin_float_native_d_b4.bin order=native

r3.out.bin --o -rd input=test_out_bin_float byte=4 null=-9999 \
    output=test_out_bin_float_native_rd_b4.bin order=native

# Test little and big endian

r3.out.bin --o input=test_out_bin_float byte=4 null=-9999 \
    output=test_out_bin_float_little_b4.bin order=little

#r3.out.bin --o input=test_out_bin_float byte=4 null=-9999 \
#    output=test_out_bin_float_big_b4.bin order=big

r3.out.bin --o input=test_out_bin_float byte=4 null=-9999 \
    output=test_out_bin_float_swap_b4.bin order=swap

    
# Write float map as integer array

r3.out.bin --o -i input=test_out_bin_float byte=1 null=0 \
    output=test_out_bin_float_native_b1_as_integer.bin order=native

r3.out.bin --o -i input=test_out_bin_float byte=2 null=0 \
    output=test_out_bin_float_native_b2_as_integer.bin order=native

r3.out.bin --o -i input=test_out_bin_float byte=4 null=0 \
    output=test_out_bin_float_native_b4_as_integer.bin order=native

r3.out.bin --o -i input=test_out_bin_float byte=8 null=0 \
    output=test_out_bin_float_native_b8_as_integer.bin order=native

# Double precision

r3.out.bin --o input=test_out_bin_double byte=4 null=-9999 \
    output=test_out_bin_double_native_b4.bin order=native

r3.out.bin --o input=test_out_bin_double byte=8 null=-9999 \
    output=test_out_bin_double_native_b8.bin order=native

# Import test floating point

r3.in.bin --o output=test_in_bin_float_1 byte=4 null=-9999 \
    input=test_out_bin_float_native_b4.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10

r3.in.bin --o output=test_in_bin_float_2 byte=8 null=-9999 \
    input=test_out_bin_float_native_b8.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10

r3.in.bin --o output=test_in_bin_float_4 byte=4 null=-9999 \
    input=test_out_bin_float_native_d_b4.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10 -d

r3.in.bin --o output=test_in_bin_float_5 byte=4 null=-9999 \
    input=test_out_bin_float_native_r_b4.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10 -r

r3.in.bin --o output=test_in_bin_float_6 byte=4 null=-9999 \
    input=test_out_bin_float_native_rd_b4.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10 -rd

# Integer binary file import

r3.in.bin --o output=test_in_bin_float_7 byte=1 null=0 \
    input=test_out_bin_float_native_b1_as_integer.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10 -i

r3.in.bin --o output=test_in_bin_float_8 byte=2 null=0 \
    input=test_out_bin_float_native_b2_as_integer.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10 -i

r3.in.bin --o output=test_in_bin_float_9 byte=4 null=0 \
    input=test_out_bin_float_native_b4_as_integer.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10 -i

r3.in.bin --o output=test_in_bin_float_10 byte=8 null=0 \
    input=test_out_bin_float_native_b8_as_integer.bin order=native \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10 -i
    
# Test little and big endian

r3.in.bin --o output=test_in_bin_float_11 byte=4 null=-9999 \
    input=test_out_bin_float_little_b4.bin order=little \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10

#r3.in.bin --o output=test_in_bin_float_12 byte=4 null=-9999 \
#    input=test_out_bin_float_big_b4.bin order=big \
#    bottom=0 top=100 west=0 east=180 south=0 north=90 \
#    cols=18 rows=9 depths=10

r3.in.bin --o output=test_in_bin_float_13 byte=4 null=-9999 \
    input=test_out_bin_float_swap_b4.bin order=swap \
    bottom=0 top=100 west=0 east=180 south=0 north=90 \
    cols=18 rows=9 depths=10

for map in `g.mlist type=rast3d pattern=test_in_bin_float*` ; do
  r3.out.ascii input=${map} output=${map}.txt dp=0
done

for i in `ls test_in_bin_float_*.txt` ; do 
    diff $i test_out_bin_float.ref
done

g.mremove -f rast3d=test_in*
g.mremove -f rast3d=test_out*
rm test_in_*.txt
rm test_out_*.bin
