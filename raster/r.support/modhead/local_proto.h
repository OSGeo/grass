#include <sys/types.h>
#include <grass/gis.h>
/* ask_format.c */
int ask_format(char *, struct Cell_head *, off_t);

/* check_un.c */
int check_uncompressed(struct Cell_head *, off_t);

/* factors.c */
int factors(FILE *, off_t, int);

/* hitreturn.c */
int hitreturn(void);

/* row_addr.c */
int next_row_addr(int, off_t *, int);
