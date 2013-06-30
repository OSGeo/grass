#!/bin/sh

g.region w=0 e=30 s=0 n=30 b=0 t=30 res3=10

r3.mapcalc --o expr="test_neighbor_float  = float(3)"
r3.mapcalc --o expr="test_neighbor_double = double(3)"
r3.mapcalc --o expr="test_neighbor_null = null()"

# First @test with float values with @precision=2
r3.neighbors input=test_neighbor_float output=test_neighbor_float_average \
    method=average window=3,3,3 --o
r3.out.ascii dp=2 input=test_neighbor_float_average output=test_neighbor_float_average.txt

r3.neighbors input=test_neighbor_float output=test_neighbor_float_sum \
    method=sum window=3,3,3 --o
r3.out.ascii dp=2 input=test_neighbor_float_sum output=test_neighbor_float_sum.txt

# Second @test with double values
r3.neighbors input=test_neighbor_double output=test_neighbor_double_average \
    method=average window=3,3,3 --o
r3.out.ascii dp=2 input=test_neighbor_double_average output=test_neighbor_double_average.txt

r3.neighbors input=test_neighbor_double output=test_neighbor_double_sum \
    method=sum window=3,3,3 --o
r3.out.ascii dp=2 input=test_neighbor_double_sum output=test_neighbor_double_sum.txt

# Third @test with null values

r3.neighbors input=test_neighbor_null output=test_neighbor_null_sum \
    method=sum window=3,3,3 --o
r3.out.ascii dp=2 input=test_neighbor_null_sum output=test_neighbor_null_sum.txt


