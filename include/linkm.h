#ifndef GRASS_LINKM_H
#define GRASS_LINKM_H

#ifndef FILE
#  include <stdio.h>
#endif

#define VOID_T char

#define PTR_CNT 10

struct link_head
{
    VOID_T **ptr_array;		/* array of pointers to chunks */
    int max_ptr;		/* num of chunks alloced */
    int alloced;		/* size of ptr_array */
    int chunk_size;		/* size of alloc chucks in units */
    int unit_size;		/* size of each user defined unit */
    VOID_T *Unused;		/* Unused list pointer */
    int exit_flag;		/* exit on error ? */
};

#include <grass/defs/linkm.h>

#endif
