/* flag.[ch] is a set of routines which will set up an array of bits
 ** that allow the programmer to "flag" cells in a raster map.
 **
 ** FLAG *
 ** flag_create(nrows,ncols)
 ** int nrows, ncols;
 **     opens the structure flag.  
 **     The flag structure will be a two dimensional array of bits the
 **     size of nrows by ncols.  Will initialize flags to zero (unset).
 **
 ** flag_destroy(flags)
 ** FLAG *flags;
 **     closes flags and gives the memory back to the system.
 **
 ** flag_clear_all(flags)
 ** FLAG *flags;
 **     sets all values in flags to zero.
 **
 ** flag_unset(flags, row, col)
 ** FLAG *flags;
 ** int row, col;
 **     sets the value of (row, col) in flags to zero.
 **
 ** flag_set(flags, row, col)
 ** FLAG *flags;
 ** int row, col;
 **     will set the value of (row, col) in flags to one.
 **
 ** int
 ** flag_get(flags, row, col)
 ** FLAG *flags;
 ** int row, col;
 **     returns the value in flags that is at (row, col).
 **
 ** idea by Michael Shapiro
 ** code by Chuck Ehlschlaeger
 ** April 03, 1989
 */
#include <stdio.h>
#define FLAG	struct _f_l_a_g_

FLAG {
    int nrows, ncols, leng;
    unsigned char **array;
};

#define FLAG_UNSET(flags,row,col) \
	(flags)->array[(row)][(col)>>3] &= ~(1<<((col) & 7))

#define FLAG_SET(flags,row,col) \
	(flags)->array[(row)][(col)>>3] |= (1<<((col) & 7))

#define FLAG_GET(flags,row,col) \
	(flags)->array[(row)][(col)>>3] & (1<<((col) & 7))

/* flag_clr_all.c */
int flag_clear_all(FLAG *);

/* flag_create.c */
FLAG *flag_create(int, int);

/* flag_destroy.c */
int flag_destroy(FLAG *);

/* flag_get.c */
int flag_get(FLAG *, int, int);

/* flag_set.c */
int flag_set(FLAG *, int, int);

/* flag_unset.c */
int flag_unset(FLAG *, int, int);
