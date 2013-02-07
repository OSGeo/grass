#ifndef __FLAG_H__
#define __FLAG_H__


/* flag.[ch] is a set of routines which will set up an array of bits
 ** that allow the programmer to "flag" cells in a raster map.
 **
 ** FLAG *
 ** flag_create(nrows,ncols)
 ** int nrows, ncols;
 **     opens the structure flag.  
 **     The flag structure will be a two dimensional array of bits the
 **     size of nrows by ncols.  Will initalize flags to zero (unset).
 **
 ** flag_destroy(flags)
 ** FLAG *flags;
 **     closes flags and gives the memory back to the system.
 **
 ** flag_clear_all(flags)
 ** FLAG *flags;
 **     sets all values in flags to zero.
 **
 * following 3 were changed to macros, same usage
 * 
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
#define FLAG struct _flagsss_
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

/* flag.c */
int flag_clear_all(FLAG *);
FLAG *flag_create(int, int);
int flag_destroy(FLAG *);

#endif /* __FLAG_H__ */
