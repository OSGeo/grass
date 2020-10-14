This directory contains scripts to check some functionality of GRASS GIS.

GRASS GIS testsuite documentation: https://grass.osgeo.org/grass79/manuals/libpython/gunittest_testing.html

## Simple test data

Some tests may be launched in the location `../demolocation/`:

```
# create new mapset for test
grass79 ../demolocation/user1 -c
# run the test
make
```

## Extended test data

Most tests require the North Carolina Sample dataset, available from
https://grass.osgeo.org/sampledata/north_carolina/

## Notes

Since 2020: For a more advanced test suite, see
 https://github.com/OSGeo/grass/actions

Until 2019: For a more advanced test suite, see
 http://fatra.cnr.ncsu.edu/grassgistests/summary_report/

