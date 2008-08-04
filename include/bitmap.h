#ifndef __GRASS_BITMAP__
#define __GRASS_BITMAP__

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

/* bitmap.c */
struct BM *BM_create(int, int);
int BM_destroy(struct BM *);
int BM_set_mode(int, int);
int BM_set(struct BM *, int, int, int);
int BM_get(struct BM *, int, int);
int BM_get_map_size(struct BM *);
int BM_file_write(FILE *, struct BM *);
struct BM *BM_file_read(FILE *);

/* sparse.c */
struct BM *BM_create_sparse(int, int);
int BM_destroy_sparse(struct BM *);
int BM_set_sparse(struct BM *, int, int, int);
int BM_get_sparse(struct BM *, int, int);
int BM_get_map_size_sparse(struct BM *);
int BM_dump_map_sparse(struct BM *);
int BM_dump_map_row_sparse(struct BM *, int);
int BM_file_write_sparse(FILE *, struct BM *);

#endif /*  __GRASS_BITMAP__  */
