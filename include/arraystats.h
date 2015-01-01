#ifndef GRASS_ARRAYSTATS_H
#define GRASS_ARRAYSTATS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>

struct GASTATS
{
    double count;
    double min;
    double max;
    double sum;
    double sumsq;
    double sumabs;
    double mean;
    double meanabs;
    double var;
    double stdev;
};

#define CLASS_INTERVAL 1
#define CLASS_STDEV    2
#define CLASS_QUANT    3
#define CLASS_EQUIPROB 4
#define CLASS_DISCONT  5
      
#include <grass/defs/arraystats.h>

#endif
