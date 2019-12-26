
/** idea by Michael Shapiro
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


/* flag.[ch] is a set of routines which will set up an array of bits
 ** that allow the programmer to "flag" cells in a raster map.
 **/
FLAG *FlagCreate(int nrows, int ncols);

/**	opens the structure flag.  
**	The flag structure will be a two dimensional array of bits the
**	size of nrows by ncols.  Will initialize flags to zero (unset).
**/
void FlagDestroy(FLAG * flags);

/**	closes flags and gives the memory back to the system.
**/
void FlagClearAll(FLAG * flags);

/**	sets all values in flags to zero.
**/
void FlagUnset(FLAG * flags, int row, int col);

/**	sets the value of (row, col) in flags to zero.
**/
void FlagSet(FLAG * flags, int row, int col);

/**	will set the value of (row, col) in flags to one.
**/
int FlagGet(FLAG * flags, int row, int col);

/**	returns the value in flags that is at (row, col).
**/
