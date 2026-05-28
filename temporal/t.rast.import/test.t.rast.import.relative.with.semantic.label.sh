#!/bin/sh

############################################################################
#
# MODULE: t.rast.import
# AUTHOR(S):	Tomas Zigo
#
# PURPOSE:	Test the import of space time raster datasets with relative time
#           and with semantic label
# COPYRIGHT:	(C) 2011-2024, Tomas Zigo and the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

export GRASS_OVERWRITE=1

# We need to set a specific region in the
# @preprocess step of this test. We generate
# raster with r.mapcalc and create a space time raster datasets
# The region setting should work for UTM and LL test locations
g.region s=0 n=80 w=0 e=120 b=0 t=50 res=10 res3=10 -p3

mkdir test

# Generate data
r.mapcalc expr="prec_1 = rand(0, 550)"
r.mapcalc expr="prec_2 = rand(0, 450)"
r.mapcalc expr="prec_3 = rand(0, 320)"
r.mapcalc expr="prec_4 = rand(0, 510)"
r.mapcalc expr="prec_5 = rand(0, 300)"
r.mapcalc expr="prec_6 = rand(0, 650)"

n1=`g.tempfile pid=1 -d`

cat > "${n1}" << EOF
prec_1|1|2|S2_1
prec_2|2|3|S2_2
prec_3|3|4|S2_3
prec_4|4|5|S2_4
prec_5|5|6|S2_5
prec_6|6|7|S2_6
EOF

eval `g.gisenv`

t.create type=strds temporaltype=relative output=precip_rel \
    title="A test with input files" descr="A test with input files"

# The first @test
t.register type=raster input=precip_rel file="${n1}" unit="years"

t.rast.export input=precip_rel output=strds_export.tar.bz2 compression=bzip2 format=GTiff directory=test

# Checking different flags
t.rast.import --o input=strds_export.tar.bz2 output=precip_rel directory=test\
          -oe title="A test" description="Description of a test"
r.info prec_1

# Check the raster maps semantic label
[[ ! "$(r.semantic.label prec_1 operation=print)" == "S2 Visible (Coastal/Aerosol)" ]] && echo "Semantic label of prec_1 raster map is not equal." && exit 1
[[ ! "$(r.semantic.label prec_2 operation=print)" == "S2 Visible (Blue)" ]] && echo "Semantic label of prec_2 raster map is not equal." && exit 1
[[ ! "$(r.semantic.label prec_3 operation=print)" == "S2 Visible (Green)" ]] && echo "Semantic label of prec_3 raster map is not equal." && exit 1
[[ ! "$(r.semantic.label prec_4 operation=print)" == "S2 Visible (Red)" ]] && echo "Semantic label of prec_4 raster map is not equal." && exit 1
[[ ! "$(r.semantic.label prec_5 operation=print)" == "S2 Vegetation Red Edge 1" ]] && echo "Semantic label of prec_5 raster map is not equal." && exit 1
[[ ! "$(r.semantic.label prec_6 operation=print)" == "S2 Vegetation Red Edge 2" ]] && echo "Semantic label of prec_6 raster map is not equal." && exit 1

# Cleaning up
t.unregister type=raster maps=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
t.remove type=strds input=precip_rel
g.remove -f type=raster name=prec_1,prec_2,prec_3,prec_4,prec_5,prec_6
rm -rf test
rm strds_export.tar.bz2
