#!/bin/sh

############################################################################
#
# TEST:         test_topology_vgeneralize
# AUTHOR(S):    Markus Metz
#
# PURPOSE:      Tests if the topology is correct using v.generalize debug mode
# COPYRIGHT:    (C) 2011-2014 by Markus Metz and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# The vectors in the sample datasets are "too good", I did not find a
# vector to provoke any errors, thus the r.to.vect step.

# Real world datasets, particularly vectors with administrative areas or
# land cover/land use classification, are in this respect more suitable
# because they contain lots of topological errors. Unfortunately, these
# datasets are too large to be included in the sample datasets.

# the script expects to run using shell with -e flag
# (equivalent of set -e in a script)

# prepare
g.region rast=landuse
r.to.vect input=landuse output=landuse type=area

# use the v.generalize debug mode
export GRASS_VECTOR_TOPO_DEBUG=1

# test will fail if this (and any other) command fails
v.generalize input=landuse output=landuse_dp method=douglas threshold=21

# clean up (executed if successful)
g.remove -f type=vector name=landuse
g.remove -f type=raster name=landuse_dp
