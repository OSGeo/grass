# Tests for r3.stats

# We set up a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc. The region setting 
# should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3
# We create several (float, double, null value) voxel map
# with value = col + row + depth. 
r3.mapcalc --o expr="volume_float = float(col() + row() + depth())"
r3.mapcalc --o expr="volume_double = double(col() + row() + depth())"
# Add null value information to test correct null value handling
r3.mapcalc --o expr="volume_float_null = if(row() == 1 || row() == 5, null(), volume_float)"
r3.mapcalc --o expr="volume_double_null = if(row() == 2 || row() == 6, null(), volume_double)"

# @test r3.stats with float maps and @file validation. 
# The module output on stdout is piped into text files
r3.stats input=volume_float_null nsteps=1 > test_volume_float_stats_1.txt
r3.stats input=volume_float_null nsteps=2  > test_volume_float_stats_2.txt
r3.stats input=volume_float_null nsteps=7  > test_volume_float_stats_7.txt
r3.stats input=volume_float_null nsteps=14 > test_volume_float_stats_14.txt
r3.stats input=volume_float_null nsteps=21 > test_volume_float_stats_21.txt
r3.stats -e input=volume_float_null > test_volume_float_stats_e.txt
# Test double maps
r3.stats input=volume_double_null nsteps=1  > test_volume_double_stats_1.txt
r3.stats input=volume_double_null nsteps=3  > test_volume_double_stats_3.txt
r3.stats input=volume_double_null nsteps=9  > test_volume_double_stats_9.txt
r3.stats input=volume_double_null nsteps=18 > test_volume_double_stats_18.txt
r3.stats input=volume_double_null nsteps=22 > test_volume_double_stats_22.txt
r3.stats -e input=volume_double_null > test_volume_double_stats_e.txt

# Comparison of references and text files
for i in `ls *.ref` ; do 
    diff $i "`basename $i .ref`.txt" ; 
done
rm *.txt
