#ifndef DATA_TYPES_H
#define DATA_TYPES_H

struct vertex
{
    double x, y, z;
    struct edge *entry_pt;
};

struct edge
{
    struct vertex *org;
    struct vertex *dest;
    struct edge *onext;
    struct edge *oprev;
    struct edge *dnext;
    struct edge *dprev;
};

#endif
