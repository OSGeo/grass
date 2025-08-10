#ifndef GRASS_BITMAPDEFS_H
#define GRASS_BITMAPDEFS_H

/* bitmap.c */
struct BM *BM_create(int, int);
int BM_destroy(struct BM *);
int BM_set_mode(int, int);
int BM_set(struct BM *, int, int, int);
int BM_get(struct BM *, int, int);
size_t BM_get_map_size(struct BM *);
int BM_file_write(FILE *, struct BM *);
struct BM *BM_file_read(FILE *);

/* sparse.c */
struct BM *BM_create_sparse(int, int);
int BM_destroy_sparse(struct BM *);
int BM_set_sparse(struct BM *, int, int, int);
int BM_get_sparse(struct BM *, int, int);
size_t BM_get_map_size_sparse(struct BM *);
int BM_dump_map_sparse(struct BM *);
int BM_dump_map_row_sparse(struct BM *, int);
int BM_file_write_sparse(FILE *, struct BM *);

#endif /*  GRASS_BITMAPDEFS_H  */
