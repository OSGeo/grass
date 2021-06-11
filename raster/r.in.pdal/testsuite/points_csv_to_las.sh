#!/bin/sh
pdal translate -i $1 -o $2 -r text -w las --writers.las.format="0" --writers.las.extra_dims="all" --writers.las.minor_version=4
