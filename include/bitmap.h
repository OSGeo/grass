#ifndef GRASS_BITMAP_H
#define GRASS_BITMAP_H

#define BM_MAGIC  2

#define BM_TEXT      "BITMAP"
#define BM_TEXT_LEN  6

#define BM_FLAT      0
#define BM_NOTSPARSE 0
#define BM_SPARSE    1

#ifndef GRASS_LINKM_H
#include <grass/linkm.h>
#endif

struct BM
{
    int rows;
    int cols;
    int bytes;
    unsigned char *data;
    int sparse;
    /* char *token; */
    struct link_head *token;
};


struct BMlink
{
    short count;
    char val;
    struct BMlink *next;
};

#ifndef _STDIO_H
#include <stdio.h>
#endif

#include <grass/defs/bitmap.h>

#endif /*  GRASS_BITMAP_H  */
