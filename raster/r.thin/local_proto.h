#include <grass/raster.h>

/* io.c */
CELL *get_a_row(int);
int put_a_row(int, CELL *);
int read_row(int, char *, int, int);
int write_row(int, char *, int, int);
int open_file(char *);
int close_file(char *);
int map_size(int *, int *, int *);

/* thin_lines.c */
int thin_lines(int);
unsigned char encode_neighbours(CELL *, CELL *, CELL *, int, int);
