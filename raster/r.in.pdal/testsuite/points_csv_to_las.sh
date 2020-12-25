#!/bin/sh
pdal translate -i data/points.csv -o points.las -r text -w las --writers.las.format="0" --writers.las.extra_dims="all" --writers.las.minor_version=4
