# The module r3.retile is the object of ths test case


# We need to set a specific region in the
# @preprocess step of this test. We generate
# voxel data with r3.mapcalc. The region setting 
# should work for UTM and LL test locations
g.region s=0 n=47 w=0 e=35 b=0 t=23 res=10 res3=1 -p3
# Now create a voxel map with value = col + row + depth. 
r3.mapcalc --o expr="volume = col() + row() + depth()"
# Add null value information
r3.mapcalc --o expr="volume_null = if(row() == 1 || row() == 5, null(), volume)"

# We @test the retiling with different tile sizes, with and without tile caching.
# The validation is based on @raster3d maps and r3.info generated @files. 
# We use @precision=0 for data export
r3.retile --o input=volume_null output=test_retile_map_1  tiledim=1x1x1
r3.info -g map=test_retile_map_1 > test_retile_map_1_info.txt
r3.retile --o input=volume_null output=test_retile_map_2  tiledim=1x1x1 -c
r3.info -g map=test_retile_map_2 > test_retile_map_2_info.txt
r3.retile --o input=volume_null output=test_retile_map_3  tiledim=8x8x4 
r3.info -g map=test_retile_map_3 > test_retile_map_3_info.txt
r3.retile --o input=volume_null output=test_retile_map_4  tiledim=8x8x4 -c
r3.info -g map=test_retile_map_4 > test_retile_map_4_info.txt
r3.retile --o input=volume_null output=test_retile_map_5  tiledim=16x16x16 
r3.info -g map=test_retile_map_5 > test_retile_map_5_info.txt
r3.retile --o input=volume_null output=test_retile_map_6  tiledim=16x16x16 -c
r3.info -g map=test_retile_map_6 > test_retile_map_6_info.txt
r3.retile --o input=volume_null output=test_retile_map_7  tiledim=35x47x23 
r3.info -g map=test_retile_map_7 > test_retile_map_7_info.txt
r3.retile --o input=volume_null output=test_retile_map_8  tiledim=35x47x23 -c
r3.info -g map=test_retile_map_8 > test_retile_map_8_info.txt
r3.retile --o input=volume_null output=test_retile_map_9  tiledim=34x46x22 
r3.info -g map=test_retile_map_9 > test_retile_map_9_info.txt
r3.retile --o input=volume_null output=test_retile_map_10 tiledim=34x46x22 -c
r3.info -g map=test_retile_map_10 > test_retile_map_10_info.txt

# Create the validation files
for map in `g.mlist type=rast3d pattern=test_retile_map_*` ; do
  r3.out.ascii input=${map} output=${map}.txt dp=0
done

# Comparison of references and text files
for i in `ls *.ref` ; do 
    diff $i "`basename $i .ref`.txt" ; 
done
rm *.txt
