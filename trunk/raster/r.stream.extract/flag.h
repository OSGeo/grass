#ifndef __FLAG_H__
#define __FLAG_H__

/* a set of routines that allow the programmer to "flag" cells in a
 * raster map. A flag is of type unsigned char, i.e. 8 bits can be set. 
 *
 * int flag_set(flag, bitno)
 *     sets the flag at position bitno to one.
 *
 * int flag_unset(flag, bitno)
 *     sets the flag at position bitno to zero.
 *
 * int flag_get(flag, bitno)
 *     checks if the flag is set at postion bitno.
 *
 * Examples:
 * set flag at position 0: FLAG_SET(flag, 0)
 * unset (clear) flag at position 7: FLAG_UNSET(flag, 7)
 * check flag at position 5: is_set_at_5 = FLAG_GET(flag, 5)
 */

/* flag positions */
#define NULLFLAG         0      /* elevation is NULL */
#define EDGEFLAG         1      /* edge cell */
#define INLISTFLAG       2      /* in open A* list */
#define WORKEDFLAG       3      /* in closed A* list/ accumulation done */
#define STREAMFLAG       4      /* stream */
#define DEPRFLAG         5      /* real depression */
#define WORKED2FLAG      6      /* extraction done */
/* last bit is unused */

#define FLAG_SET(flag,bitno) ((flag) |= (1 << (bitno)))
#define FLAG_UNSET(flag,bitno) ((flag) &= ~(1 << (bitno)))
#define FLAG_GET(flag,bitno) ((flag) & (1 << (bitno)))

#endif /* __FLAG_H__ */
